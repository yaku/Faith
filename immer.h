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
        unsigned long long int size;
        unsigned int skip;

} device;

typedef struct space {

        unsigned long long int begin;
        unsigned long long int end;

} space;

int is_free(device *device, unsigned long long int adress, unsigned int length) 
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
        
    
        return flag;
    
        free(buffer);
    

}

void fill_random(device *device, unsigned int blocks, int type) 
{ 

        lseek(device->descriptor, device->skip, SEEK_SET);
    
        if (type == DEVURANDOM) {
    
                printf("\nFilling device by random data with system urandom randomizer\n");
    
                int urandom = open("/dev/urandom",O_RDONLY);
    
                if (urandom < 0)
                        pdie("Opening pseudorandom numbers generator failed");
    
                unsigned char *buffer = (unsigned char *) malloc(BLOCKSIZE);
    
                while (blocks--) {
        
                        if (read(urandom, buffer, BLOCKSIZE) < 0)
                                pdie("Read failed");
            
                        if (write(device->descriptor, buffer, BLOCKSIZE) < 0)
                                pdie("Write failed");
    
                }
        }
    
        if (type == SLRAND) {
    
                printf("\nFilling device by random data with standart library randomizer\n");
    
                int i = 0;
                unsigned char *randomblock = malloc(BLOCKSIZE);
        
                while (blocks--) {
                        for(i = 0; i < BLOCKSIZE; i++)
                                *(randomblock+i) = rand() % 256;
                
                        if (write(device->descriptor, randomblock, BLOCKSIZE) < 0)
                                pdie("Write failed");
                }
        
                free(randomblock);
        }

}

void fill_zero(device *device, unsigned int blocks) 
{

        lseek(device->descriptor, device->skip, SEEK_SET);
        unsigned char *zeros = (unsigned char *) malloc(BLOCKSIZE);
        memset(zeros, 0x00, BLOCKSIZE);
    
        while (blocks--)
                if (write(device->descriptor, zeros, BLOCKSIZE) < 0)
                        pdie("Write failed");
    
        free(zeros);

}

void write_data(device *device, void *block, unsigned int length, unsigned long long int address) 
{

        lseek(device->descriptor, address + device->skip, SEEK_SET);
        if (write(device->descriptor, block, length) < 0)
                pdie("Write failed");

}

void read_data(device *device, void *block, unsigned int length, unsigned long long int address) 
{

        lseek(device->descriptor, address + device->skip, SEEK_SET);
        if (read(device->descriptor, block, length) < 0)
                pdie("Read failed");

}

void set_flags(device *device, unsigned long long int address, unsigned int length) 
{

        unsigned char *flags = (unsigned char *) malloc(length);
        memset(flags, 0x80, length);
    
        lseek(device->descriptor, address + device->skip, SEEK_SET);
        if (write(device->descriptor, flags, length) < 0)
                pdie("Write failed");
    
        free(flags);
    
}

void skey2archive(unsigned long long int *skey, unsigned char *buffer, unsigned int skeysize) 
{

        int i = 0;
        for (i = 0; i < skeysize; i++)
                memcpy(buffer + i * SKEYRECORDSIZE, skey + i, SKEYRECORDSIZE);

}

void archive2skey(unsigned long long int *skey, unsigned char *buffer, unsigned int skeysize) 
{

        int i = 0;
        for (i = 0; i < skeysize; i++)
                memcpy(skey + i, buffer + i * SKEYRECORDSIZE, SKEYRECORDSIZE);

}

unsigned long long int get_next_block_address(device *device, unsigned long long address, int direction, int inblock) 
{

        int offset = 0;
        int i = 0;
        int flag = 0;
        int blockcounter = 0;
    
        unsigned char *block = malloc(BLOCKSIZE);
    
        if (direction == FORWARD) {
                
                while (!flag) {
                
                        read_data(device, block, BLOCKSIZE, address + BLOCKSIZE * blockcounter);
        
                        for (i = 0; i < BLOCKSIZE; i++) {
                                
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
                
                        read_data(device, block, BLOCKSIZE, address - BLOCKSIZE * (blockcounter + 1));
        
                        for (i = BLOCKSIZE - 1; i > 0; i--) {
        
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

void get_next_space(device *device, unsigned long long address, int direction, space *space) 
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

unsigned long long int get_skey_in_space(device *device, space *space, int blocksize) 
{

        int subrnd = 0;
                
        int flag = 0;
        
        unsigned long long int result = 0;
                
        int max = space->end - space->begin;
               
        while (!flag) {
                
                subrnd = rand() % max;
                    
                if (is_free(device, space->begin + subrnd, blocksize)) {
            
                        result = space->begin + subrnd;
                        set_flags(device, space->begin + subrnd, blocksize);
                    
                        flag = 1;
            
                } else if (is_free(device, space->begin + subrnd - blocksize, blocksize)) {
            
                        result = space->begin + subrnd - blocksize;
                        set_flags(device, space->begin + subrnd - blocksize, blocksize);

                        flag = 1;
            
                } else {
                    
                        max = subrnd;
                    
                }      
                
        }
        
        return result;

}


void make_skey(device *device, unsigned long long int *skey, unsigned int keysize, int blocksize) 
{

        unsigned long long int rnd = 0;
        unsigned int completed = 0;
        int level = 0;
    
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
                           get_next_block_address(device, rnd, BACKWARD, 0) > blocksize) {
                        
                        space space = { 
                        
                                get_next_block_address(device, rnd, BACKWARD, 0),
                                get_next_block_address(device, rnd, FORWARD, 0)
                        
                        };
                        
                        *skey++ = get_skey_in_space(device, &space, blocksize);
                        
                        completed++;
                        level = 0;
                        printf("USE\n");
                        
     
                }  else {
                /*
                        space space;
                        
                        printf("R %llu\t",rnd);
                        
                        get_next_space(device, get_next_block_address(device, rnd, FORWARD, 0), FORWARD, &space);
                        printf("FWD %llu\t", space.end - space.begin);
                        
                        get_next_space(device, get_next_block_address(device, rnd, BACKWARD, 0), BACKWARD, &space);
                        printf("BKWD %llu\n",space.end - space.begin);
            */
                        level += 1;
            
                }

                if (level == MAXLEVEL) {
                /*
                        int directionflag=0b100;
                        unsigned long long int next = get_next_block_address(device, rnd, FORWARD, 0);
                        unsigned long long int previous = get_next_block_address(device, rnd, BACKWARD, 0);
                        
                        space nextspace;
                        space previousspace;
                        
                        int ok = 0;
                        
                        while (directionflag != 0b11) {
                        
                                switch (directionflag) 
                                case 0b01: {
                                        
                                        while (!ok) {
                                                
                                                
                                                get_next_space(device, previous, BACKWARD, &previousspace);
                                                
                                                if (previousspace.begin < 1024)
                                                        pdie("Near zero");
                                                
                                                if ((previousspace.end - previousspace.end) > blocksize) {
                                              
                                                        int subrnd = 0;
                
                                                        int flag = 0;
                
                                                        int max = previousspace.end - previousspace.begin;
               
                                                        while (!flag) {
                
                                                                subrnd = rand() % max;
                    
                                                                if (is_free(device, previous + subrnd, blocksize)) {
            
                                                                        *skey++ = previous + subrnd;
                                                                        completed++;
                                                                        set_flags(device, previous + subrnd, blocksize);
                    
                                                                        level = 0;
                                                                        flag = 1;
            
                                                                } else if (is_free(device, previous + subrnd - blocksize, blocksize)) {
            
                                                                        *skey++ = previous + subrnd - blocksize;
                                                                        completed++;
                                                                        set_flags(device, previous + subrnd - blocksize, blocksize);
                    
                                                                        level = 0; 
                                                                        flag = 1;
            
                                                                } else {
                    
                                                                        max = subrnd;
                    
                                                                }      
                
                                                        }
                                                        
                                                        ok = 1;
                                               
                                                }
                                                
                                                
                                                
                                                
                                        }
                                
                                
                                }
                        
                        }
        */
                        printf("Input is too big for this device\nExiting.\n");
                        exit(1);
        
                }
    
        }
    

}

void make_skey_main(device *device, int skeyfile, unsigned int keysize) 
{
    
        unsigned int i = 0; 
        unsigned long long int skey = 0;
        unsigned char *buffer = calloc(1, SKEYRECORDSIZE);

        for (i = 0; i < keysize; i++) {
    
                make_skey(device, &skey, 1, BLOCKSIZE);
                skey2archive(&skey, buffer, 1);
                write(skeyfile, buffer, SKEYRECORDSIZE);

        }
    
        free(buffer);

}

void write_by_skey(device *device, int datafile, int skeyfile, unsigned int blocks) 
{

        unsigned long long int skey = 0;
        unsigned char *buffer = calloc(1, SKEYRECORDSIZE);
    
        unsigned char *block = calloc(BLOCKSIZE, 1);
    
        int buffercount = 0;
        int i = 0;
    
        for (i = 0; i < blocks; i++) {

                read(skeyfile, buffer, SKEYRECORDSIZE);
                archive2skey(&skey, buffer, 1);

                read(datafile, block, BLOCKSIZE);
                write_data(device, block, BLOCKSIZE, skey);

        }
    
        free(buffer);
        free(block);

}

void get_data(device *device, int datafile, unsigned long long int skeyaddress, unsigned int skeysize) 
{

        unsigned long long int skey = 0;
        unsigned char *buffer = calloc(1, SKEYRECORDSIZE);
    
        unsigned char *block = calloc(BLOCKSIZE, 1);
    
        int i = 0;
    
        for(i = 0; i < skeysize; i++) {
    
                read_data(device, buffer, SKEYRECORDSIZE, skeyaddress + i * SKEYRECORDSIZE);
                archive2skey(&skey, buffer, 1);

                read_data(device, block, BLOCKSIZE, skey);
                write(datafile, block, BLOCKSIZE);    
        }
    
        free(buffer);
        free(block);
    
}

unsigned long long int file_size(unsigned char *filename) 
{

        struct stat filestat;
        stat(filename, &filestat);
    
        return filestat.st_size;

}

unsigned long long int buffer_for_skey(device *device, unsigned int skeylen) 
{

        unsigned long long int result = 0;

        make_skey(device, &result, 1, skeylen * SKEYRECORDSIZE);    
    
        return result;

}

void write_skey(device *device, int skeyfile, unsigned int skeylen, unsigned long long int address) 
{

        lseek(device->descriptor, address + device->skip, SEEK_SET);
        lseek(skeyfile, 0, SEEK_SET);
    
        unsigned char *block = calloc(BLOCKSIZE, 1);
    
        int readchars = 0;
    
        while (skeylen -= BLOCKSIZE > 0) {
                if ((readchars = read(skeyfile, block, BLOCKSIZE)) < 0)
                        pdie("Read failed");
        
                if (write(device->descriptor, block, readchars) < 0)
                        pdie("Write failed");
        
        } 
    
        free(block);   

}

unsigned long long int device_size(device *device) 
{

        unsigned long long size = 0;

        #ifdef DEV
        if(ioctl(device->descriptor, BLKGETSIZE64, &size) != -1)
                ;
        else
                pdie("Getting device size failed");
        #endif
    
        #ifndef DEV
        size = 1024 * 1024 * 64;
        #endif
    
        return size;

}

unsigned long long int set_skip(device *device) /**/
{ 

        return 512;

}

void make_mbr() {}
void write_boot_partition() {}

void immer_main(int mode, char *devicename, char *datafilename, char *keyfilename, char *charpassword, char *dataskeyfilename, char *keyskeyfilename) 
{

        device device;
    
        int datafile;
        int keyfile;
    
        int outdatafile;
        int outkeyfile;
    
        int dataskeyfile;
        int keyskeyfile;
    
        struct pass password = {0, 0, 0}; 
    
        unsigned int skeysize = 0;
        unsigned long long int datalen = 0;
    
        printf("Opening device...");
        device.descriptor = open(devicename, O_RDWR);
        if (device.descriptor < 0)
                pdie("Device open failed. Maybe wrong device selected?");
        
        printf(".done\n");
    
        /*Open files*/
        printf("\nOpening files...");
    
        if (mode == ENCRYPT) {

                datafile = open(datafilename, O_RDONLY);
                keyfile  = open(keyfilename, O_RDONLY);
        
                dataskeyfile = open(dataskeyfilename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                keyskeyfile  = open(keyskeyfilename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        
                if (datafile < 0 || keyfile < 0 || dataskeyfile < 0 || keyskeyfile< 0)
                        pdie("Open failed");
        
        }
    
        if (mode == DECRYPT) {
        
                outdatafile = open(datafilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                outkeyfile  = open(keyfilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                if (outdatafile < 0 || outkeyfile < 0)
                        pdie("Open failed");

        }
    
        printf(".done\n");
        /*End of Open files*/
    
        printf("Getting device size...");
        device.size = device_size(&device); 
        printf(".done %lld\n", device.size);  
    
        printf("Setting skip for mbr and boot partition...");
        device.skip = set_skip(&device);
        printf(".done %u\n", device.skip);  
    
    
        printf("Initialising random number numbers generator...");
        srand(time(NULL));
        printf(".done\n");
    
        unsigned long long int dataskeyaddress = 0;
        unsigned long long int keyskeyaddress = 0;  
    
        device.size -= device.skip;
    
        printf("Reading configuration file...");
        config conf;
        read_config(&conf, "config.cfg");
        printf(".done\n");
    
        if (mode == ENCRYPT) {
    
                printf("Calculating length of input...");
                datalen = file_size(datafilename);
                printf(".done %lld\n", datalen);
        
                printf("Calculating size of skey...");
                skeysize = datalen/BLOCKSIZE;
                printf(".done %u\n", skeysize);
    
                printf("Preparing device. All data will be destroyed...");
                fill_zero(&device, device.size / BLOCKSIZE);
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
                fill_random(&device, device.size / BLOCKSIZE, conf.randomizer);
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
                write_skey(&device, dataskeyfile, skeysize * SKEYRECORDSIZE, dataskeyaddress);
                write_skey(&device, keyskeyfile, skeysize * SKEYRECORDSIZE, keyskeyaddress);
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
   
    
        /*Close files*/

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
        /*End of close files*/

}

