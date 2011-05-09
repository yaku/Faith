/*
 * Immersion library.
 *
 * Copyright (C) 2010, 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

typedef struct device {

        int descriptor;
        u_int64_t size;
        uint skip;

} device;

typedef struct space {

        u_int64_t begin;
        u_int64_t end;

} space;

int is_free(device *device, u_int64_t adress, uint length)
{

        unsigned char *buffer = (unsigned char *) malloc(length);

        int flag = 0;
        int i = 0;

        lseek(device->descriptor, adress + device->skip, SEEK_SET);

        if (read(device->descriptor, buffer, length) > 0) {

                for (i = 0; i < length; i++) {

                        if (buffer[i] == '\0') {

                                flag = 1;

                        } else {

                                flag = 0;
                                break;

                        }
                }
        } else
                pdie("Read failed");

        free(buffer);

        return flag;
}

void fill_random(device *device, u_int64_t bytes, int type)
{

        lseek(device->descriptor, device->skip, SEEK_SET);

        bytes -= device->skip;

        unsigned char *randombuffer = malloc(BUFFER_SIZE);

        if (type == DEVURANDOM) {

                printf("\nFilling by random data with system urandom randomizer\n");

                int urandom = open("/dev/urandom",O_RDONLY);

                if (urandom < 0)
                        pdie("Opening pseudorandom numbers generator failed");

                while (bytes > BUFFER_SIZE) {

                        if (read(urandom, randombuffer, BUFFER_SIZE) < 0)
                                pdie("Read failed");

                        if (write(device->descriptor, randombuffer, BUFFER_SIZE) < 0)
                                pdie("Write failed");

                        bytes -= BUFFER_SIZE;

                }

                if (read(urandom, randombuffer, bytes) < 0)
                        pdie("Read failed");

                if (write(device->descriptor, randombuffer, bytes) < 0)
                        pdie("Write failed");
        }

        if (type == SLRAND) {

                printf("\nFilling by random data with standart library randomizer\n");

                int i = 0;

                while (bytes > BUFFER_SIZE) {

                        for(i = 0; i < BUFFER_SIZE; i++)
                                *(randombuffer+i) = rand() % 256;

                        if (write(device->descriptor, randombuffer, BUFFER_SIZE) < 0)
                                pdie("Write failed");

                        bytes -= BUFFER_SIZE;
                }

                for(i = 0; i < bytes; i++)
                        *(randombuffer+i) = rand() % 256;

                if (write(device->descriptor, randombuffer, bytes) < 0)
                        pdie("Write failed");

        }

        free(randombuffer);

}

void fill_zero(device *device, u_int64_t bytes)
{

        lseek(device->descriptor, device->skip, SEEK_SET);
        unsigned char *zeros = (unsigned char *) calloc(1, BUFFER_SIZE);

        bytes -= device->skip;

        while (bytes > BUFFER_SIZE) {
                if (write(device->descriptor, zeros, BUFFER_SIZE) < 0)
                        pdie("Write failed");

                bytes -= BUFFER_SIZE;
        }

        if (write(device->descriptor, zeros, bytes) < 0)
                pdie("Write failed");

        free(zeros);

}

void write_data(device *device, unsigned char *block, int length, u_int64_t address)
{

        extern int errno;
        int writtenchars;

        lseek(device->descriptor, address + device->skip, SEEK_SET);

        while (length != 0) {
                if ((writtenchars = write(device->descriptor, block, length)) < 0)
                        if (errno = EINTR)
                                continue;
                        else
                                pdie("Write failed");

                length -= writtenchars;
                block += writtenchars;
        }
}

void read_data(device *device, unsigned char *block, uint length, u_int64_t address)
{

        extern int errno;

        lseek(device->descriptor, address + device->skip, SEEK_SET);

        int ret = 0;

        while (length != 0) {
                if ((ret = read(device->descriptor, block, length)) < 0)
                        if (errno == EINTR)
                                continue;
                        else
                                pdie("Read failed");

                length -= ret;
                block += ret;

        }

}

void set_flags(device *device, u_int64_t address, uint length)
{

        unsigned char *flags = (unsigned char *) malloc(length);
        memset(flags, 0x80, length);

        lseek(device->descriptor, address + device->skip, SEEK_SET);
        if (write(device->descriptor, flags, length) < 0)
                pdie("Write failed");

        free(flags);

}

void skey2archive(u_int64_t *skey, unsigned char *buffer, uint skeysize)
{

        int i = 0;
        for (i = 0; i < skeysize; i++)
                memcpy(buffer + i * SKEYRECORDSIZE, skey + i, SKEYRECORDSIZE);

}

void archive2skey(u_int64_t *skey, unsigned char *buffer, uint skeysize)
{

        int i = 0;
        for (i = 0; i < skeysize; i++)
                memcpy(skey + i, buffer + i * SKEYRECORDSIZE, SKEYRECORDSIZE);

}

u_int64_t get_next_block_address(device *device, u_int64_t address, int direction, int inblock)
{

        int offset = 0;
        int i = 0;
        int flag = 0;
        int blockcounter = 0;

        unsigned char *block = malloc(STEP);

        if (direction == FORWARD) {

                while (!flag) {

                        read_data(device, block, STEP, address + STEP * blockcounter);

                        for (i = 0; i < STEP; i++) {

                                if (inblock)
                                        if (*(block + i) != 0)
                                                offset++;
                                        else {
                                                flag = 1;
                                                break;
                                        }
                                else
                                        if (*(block + i) == 0)
                                                offset++;
                                        else {
                                                flag = 1;
                                                break;
                                        }

                        }

                        blockcounter++;
                }

        }

        if (direction == BACKWARD) {

                while (!flag) {

                        read_data(device, block, STEP, address - STEP * (blockcounter + 1));

                        for (i = STEP - 1; i > 0; i--) {

                                if (inblock)
                                        if (*(block + i) != 0)
                                                offset++;
                                        else {
                                                flag = 1;
                                                break;
                                        }
                                else
                                        if (*(block + i) == 0)
                                                offset++;
                                        else {
                                                flag = 1;
                                                break;
                                        }

                        }

                        blockcounter++;
                }
                offset = -offset;

        }

        free(block);

        return address + offset;

}

void get_next_space(device *device, u_int64_t address, int direction, space *space)
{

        if (direction == FORWARD) {

                space->begin = get_next_block_address(device, address, direction, 1);
                space->end = get_next_block_address(device, space->begin, direction, 0);

        }

        if (direction == BACKWARD) {

                space->end = get_next_block_address(device, address, direction, 1);
                space->begin = get_next_block_address(device, space->end - 1, direction, 0);

        }

}

u_int64_t get_skey_in_space(device *device, space *space, int blocksize)
{

        int subrnd = 0;

        int flag = 0;

        u_int64_t result = 0;

        int max = space->end - space->begin;

        while (!flag) {

                subrnd = rand() % max;

                if ((space->end - (space->begin + subrnd)) > blocksize) {

                        result = space->begin + subrnd;
                        set_flags(device, space->begin + subrnd, blocksize);

                        flag = 1;

                } else {

                        max = subrnd;

                }

        }

        return result;

}


void make_skey(device *device, u_int64_t *skey, uint keysize, int blocksize)
{

        u_int64_t rnd = 0;
        uint completed = 0;
        int level = 0;

        int flag;
        int ok;

        space zerospace = {0, 0};

        while (completed < keysize) {

                rnd = rand() % (device->size - blocksize);

                if (is_free(device, rnd, blocksize)) {

                        *skey++ = rnd;
                        completed++;
                        set_flags(device, rnd, blocksize);
                        level = 0;

                } else if (is_free(device, rnd - blocksize, blocksize)) {

                        *skey++ = rnd - blocksize;
                        completed++;
                        set_flags(device, rnd - blocksize, blocksize);
                        level = 0;

                } else if (get_next_block_address(device, rnd, FORWARD, 0) - \
                           get_next_block_address(device, rnd, BACKWARD, 0) >= blocksize) {

                        space space = {

                                get_next_block_address(device, rnd, BACKWARD, 0),
                                get_next_block_address(device, rnd, FORWARD, 0)

                        };

                        *skey++ = get_skey_in_space(device, &space, blocksize);

                        completed++;
                        level = 0;

                }  else {

                        level += 1;

                }

                if (level == MAXLEVEL) {

                        flag = 1;
                        ok   = 0;

                        space nextspace;

                        while (flag && !ok) {

                                nextspace = zerospace;

                                while (!ok) {

                                        get_next_space(device, nextspace.end, FORWARD, &nextspace);

                                        if ((device->size - nextspace.begin) < 1024) {

                                                printf("Error: Near EOF\n");
                                                flag = 0;
                                                break;

                                        }

                                        if ((nextspace.end - nextspace.begin) >= blocksize) {

                                                *skey++ = get_skey_in_space(device, &nextspace, blocksize);
                                                ok = 1;

                                                level = 0;
                                                completed++;

                                        }

                                }
                        }



                        if (!flag) {

                                printf("Input is too big for this device\nExiting.\n");
                                exit(1);

                        }

                }

        }


}

void make_skey_main(device *device, int skeyfile, uint keysize)
{

        uint i = 0;
        u_int64_t *skey = calloc(BUFFERED_BLOCKS, 8);
        unsigned char *buffer = calloc(BUFFERED_BLOCKS, SKEYRECORDSIZE);

        while (keysize > BUFFERED_BLOCKS) {

                make_skey(device, skey, BUFFERED_BLOCKS, BLOCKSIZE);
                skey2archive(skey, buffer, BUFFERED_BLOCKS);

                if (write(skeyfile, buffer, BUFFERED_BLOCKS * SKEYRECORDSIZE) < 0)
                        pdie("Write failed");

                keysize -= BUFFERED_BLOCKS;

        }

        make_skey(device, skey, keysize, BLOCKSIZE);
        skey2archive(skey, buffer, keysize);

        if (write(skeyfile, buffer, keysize * SKEYRECORDSIZE) < 0)
                pdie("Write failed");

        free(buffer);
        free(skey);

}

void write_by_skey(device *device, int datafile, int skeyfile, uint blocks)
{

        u_int64_t *skey = calloc(BUFFERED_BLOCKS, 8);
        unsigned char *buffer = calloc(BUFFERED_BLOCKS, SKEYRECORDSIZE);

        unsigned char *block = calloc(BLOCKSIZE, BUFFERED_BLOCKS);

        int i;

        while (blocks > BUFFERED_BLOCKS) {

                if (read(skeyfile, buffer, SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Read failed");

                archive2skey(skey, buffer, BUFFERED_BLOCKS);

                if (read(datafile, block, BLOCKSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Read failed");

                for (i = 0; i < BUFFERED_BLOCKS; i++)
                        write_data(device, block + i * BLOCKSIZE, BLOCKSIZE, *(skey + i));

                blocks -= BUFFERED_BLOCKS;
        }

        if (read(skeyfile, buffer, SKEYRECORDSIZE * blocks) < 0)
                pdie("Read failed");

        archive2skey(skey, buffer, blocks);

        if (read(datafile, block, BLOCKSIZE * blocks) < 0)
                pdie("Read failed");

        for (i = 0; i < blocks; i++)
                write_data(device, block + i * BLOCKSIZE, BLOCKSIZE, *(skey + i));

        free(skey);
        free(buffer);
        free(block);

}

void get_data(device *device, int datafile, u_int64_t skeyaddress, uint keysize)
{

        u_int64_t *skey = calloc(BUFFERED_BLOCKS, 8);
        unsigned char *buffer = calloc(BUFFERED_BLOCKS, SKEYRECORDSIZE);

        unsigned char *block = calloc(BLOCKSIZE, BUFFERED_BLOCKS);

        int i;

        while (keysize > BUFFERED_BLOCKS) {

                read_data(device, buffer, BUFFERED_BLOCKS * SKEYRECORDSIZE, skeyaddress);

                archive2skey(skey, buffer, BUFFERED_BLOCKS);

                for (i = 0; i < BUFFERED_BLOCKS; i++)
                        read_data(device, block + i * BLOCKSIZE, BLOCKSIZE, *(skey + i));

                if (write(datafile, block, BLOCKSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Write failed");

                skeyaddress += BUFFERED_BLOCKS * SKEYRECORDSIZE;

                keysize -= BUFFERED_BLOCKS;
        }

        read_data(device, buffer, keysize * SKEYRECORDSIZE, skeyaddress);

        archive2skey(skey, buffer, keysize);

        for (i = 0; i < keysize; i++)
                read_data(device, block + i * BLOCKSIZE, BLOCKSIZE, *(skey + i));

        if (write(datafile, block, BLOCKSIZE * keysize) < 0)
                pdie("Write failed");

        free(skey);
        free(buffer);
        free(block);

}

u_int64_t file_size(char *filename)
{

        struct stat filestat;
        stat(filename, &filestat);

        return filestat.st_size;

}

u_int64_t ffile_size(int fd)
{

        struct stat filestat;
        fstat(fd, &filestat);

        return filestat.st_size;

}

u_int64_t buffer_for_skey(device *device, uint skeylen)
{

        u_int64_t result = 0;

        make_skey(device, &result, 1, skeylen * SKEYRECORDSIZE);

        return result;

}

void write_skey(device *device, int skeyfile, uint skeylen, u_int64_t address)
{

        lseek(device->descriptor, address + device->skip, SEEK_SET);
        lseek(skeyfile, 0, SEEK_SET);

        unsigned char *block = calloc(SKEYRECORDSIZE, BUFFERED_BLOCKS);

        int readchars = 0;

        while (skeylen > BUFFERED_BLOCKS) {

                if (read(skeyfile, block, SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Read failed");

                if (write(device->descriptor, block, SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Write failed");

                skeylen -= BUFFERED_BLOCKS;
        }

        if (read(skeyfile, block, SKEYRECORDSIZE * skeylen) < 0)
                pdie("Read failed");

        if (write(device->descriptor, block, SKEYRECORDSIZE * skeylen) < 0)
                pdie("Write failed");

        free(block);

}

u_int64_t device_size(device *device)
{

        u_int64_t size = 0;

        if(ioctl(device->descriptor, BLKGETSIZE64, &size) != -1)
                ;
        else
                pdie("Getting device size failed");

        return size;

}

u_int64_t set_skip(device *device) /**/
{

        return 512;

}

void immer_main(int mode, char *devicename, names filename, char *charpassword, config conf)
{

        device device;

        int datafile;
        int keyfile;

        int outdatafile;
        int outkeyfile;

        int dataskeyfile;
        int keyskeyfile;

        struct pass password = {0, 0, 0};

        uint skeysize = 0;
        u_int64_t datalen = 0;

        printf("\nOpening device...");

        if (conf.isfile)
                device.descriptor = open(devicename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        else
                device.descriptor = open(devicename, O_RDWR);

        if (device.descriptor < 0)
                pdie("Device open failed. Maybe wrong device selected?");

        printf(".done\n");

        printf("Opening files...");

        if (mode == ENCRYPT) {

                datafile = open(filename.enc, O_RDONLY);
                keyfile  = open(filename.key, O_RDONLY);

                dataskeyfile = open(filename.dataskey, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                keyskeyfile  = open(filename.keyskey, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                if (datafile < 0 || keyfile < 0 || dataskeyfile < 0 || keyskeyfile< 0)
                        pdie("Open failed");

        }

        if (mode == DECRYPT) {

                outdatafile = open(filename.data, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                outkeyfile  = open(filename.key, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                if (outdatafile < 0 || outkeyfile < 0)
                        pdie("Open failed");

        }

        printf(".done\n");

        printf("Setting skip for mbr and boot partition...");
        device.skip = set_skip(&device);
        printf(".done %u\n", device.skip);

        u_int64_t dataskeyaddress = 0;
        u_int64_t keyskeyaddress = 0;

        if (mode == ENCRYPT) {

                printf("Getting device size...");
                if (conf.isfile)
                        device.size = conf.filesize;
                else
                        device.size = device_size(&device);

                printf(".done %lld\n", device.size);

                device.size -= device.skip;

                printf("Calculating length of input...");
                datalen = file_size(filename.data);
                printf(".done %lld\n", datalen);

                printf("Calculating size of skey...");
                skeysize = datalen/BLOCKSIZE;
                printf(".done %u\n", skeysize);

                printf("Preparing device. All data will be destroyed...");
                fill_zero(&device, device.size);
                puts(".done\n");

                printf("Setting buffers for data skey and key skey...");
                dataskeyaddress = buffer_for_skey(&device, skeysize);
                keyskeyaddress  = buffer_for_skey(&device, skeysize);
                printf(".done\n");

                printf("Making skey for data...");
                make_skey_main(&device, dataskeyfile, skeysize);
                printf(".done\n");

                printf("Making skey for key...");
                make_skey_main(&device, keyskeyfile, skeysize);
                printf(".done\n");

                printf("Filling device with random data...");
                fill_random(&device, device.size, conf.randomizer);
                printf(".done\n");

                lseek(dataskeyfile, 0, SEEK_SET);
                lseek(keyskeyfile, 0, SEEK_SET);

                printf("Writing data by skey...");
                write_by_skey(&device, datafile, dataskeyfile, skeysize);
                printf(".done\n");

                printf("Writing key by skey...");
                write_by_skey(&device, keyfile, keyskeyfile, skeysize);
                printf(".done\n");

                printf("Writing skey's...");
                write_skey(&device, dataskeyfile, skeysize, dataskeyaddress);
                write_skey(&device, keyskeyfile, skeysize, keyskeyaddress);
                printf(".done\n");

                unsigned char *outpassword = calloc(20, 1);
                make_password(outpassword, skeysize, dataskeyaddress, keyskeyaddress);

                printf("Your's password: %s\n", outpassword);

                free(outpassword);
        }

        if (mode == DECRYPT) {

                printf("Preparing password...");
                prepare_password(charpassword, &password);
                printf(".done\n");

                printf("Getting size of skey...");
                skeysize = password.skeysize;
                printf(".done %u\n", skeysize);

                printf("Getting data by skey...");
                get_data(&device, outdatafile, password.dataskeyaddress, skeysize);
                printf(".done\n");

                printf("Getting key by skey...");
                get_data(&device, outkeyfile, password.keyskeyaddress, skeysize);
                printf(".done\n");
        }

        close(device.descriptor);
        if (mode == ENCRYPT) {
                close(datafile);
                close(keyfile);

                close(dataskeyfile);
                close(keyskeyfile);
        }

        if (mode == DECRYPT) {
                close(outdatafile);
                close(outkeyfile);
        }

}

