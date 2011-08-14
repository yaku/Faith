/*
 * Immersion library.
 *
 * Copyright (C) 2010, 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */
 
 typedef unsigned char uchar;
 
 /* Globals. Need fix */
 int space1fd;
 int space2fd;

typedef struct device {

        int descriptor;
        u_int64_t size;

} device;

typedef struct space {

        u_int64_t begin;
        u_int64_t end;

} space;

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

void fill_random(device *activedev, u_int64_t bytes)
{

        lseek(activedev->descriptor, 0, SEEK_SET);

        uchar *randombuffer = (uchar *) malloc(BUFFER_SIZE);

        printf("\nFilling by random data with system urandom randomizer\n");

        int urandom = open("/dev/urandom",O_RDONLY);

        if (urandom < 0)
	        pdie("Opening pseudorandom numbers generator failed");

        while (bytes > BUFFER_SIZE) {

	        if (read(urandom, randombuffer, BUFFER_SIZE) < 0)
	                pdie("Read failed");

                if (write(activedev->descriptor, randombuffer, BUFFER_SIZE) < 0)
                        pdie("Write failed");

                bytes -= BUFFER_SIZE;

	}

        if (read(urandom, randombuffer, bytes) < 0)
	        pdie("Read failed");

	if (write(activedev->descriptor, randombuffer, bytes) < 0)
	        pdie("Write failed");

        free(randombuffer);

}

void archive2skey(u_int64_t *skey, uchar *buffer, uint skeysize)
{

        int i = 0;
        for (i = 0; i < skeysize; i++)
                memcpy(skey + i, buffer + i * SKEYRECORDSIZE, SKEYRECORDSIZE);
}

void skey2archive(u_int64_t *skey, uchar *buffer, uint skeysize)
{

        int i = 0;
        for (i = 0; i < skeysize; i++)
                memcpy(buffer + i * SKEYRECORDSIZE, skey + i, SKEYRECORDSIZE);
}

int u_int64_t_cmp(const void *long1, const void *long2)
{
	
	if (*((u_int64_t *) long1) > *((u_int64_t *) long2))
		return 1;

	else if (*((u_int64_t *) long1) == *((u_int64_t *) long1))
		return 0;

	else
		return -1;

}

int uint_cmp(const void *uint1, const void *uint2)
{

	if (*((uint *) uint1) > *((uint *) uint2))
		return 1;

	else if (*((uint *) uint1) == *((uint *) uint2))
		return 0;

	else
		return -1;

}

int int_cmp(const void *int1, const void *int2)
{

	return *((int *) int2) - *((int *) int1);

}

void read_space(int spacefile, uint index, space *outspace)
{

	uchar *buf = (uchar *) calloc(SKEYRECORDSIZE * 2, 1);
	
	if (pread(spacefile, buf, SKEYRECORDSIZE * 2, index * SKEYRECORDSIZE * 2) < 0)
		pdie("Read failed");

	outspace->begin = 0;
	outspace->end = 0;
	
	archive2skey(&(outspace->begin), buf, 1);
	archive2skey(&(outspace->end), buf + SKEYRECORDSIZE, 1);
	
	free(buf);
}

void rewrite_space(int *spacefile, uint *used, u_int64_t *skey, int lastused, int blocksize)
{
	extern int space1fd;
	extern int space2fd;
	int newspacefile; 
	
	if (*spacefile == space1fd) {

		if (ftruncate(space2fd, 0) < 0)
			pdie("Truncate failed");

		newspacefile = space2fd;		
	} else {
		
		if (ftruncate(space1fd, 0) < 0)
			pdie("Truncate failed");

		newspacefile = space1fd;
	}

	uchar *buf 	   = (uchar *) calloc(2 * SKEYRECORDSIZE * BUFFERED_BLOCKS, 1); 
	uchar *skeyarchive = (uchar *) calloc(2 * SKEYRECORDSIZE, 1);
	int lastwritten = 0;
	int towrite 	= 0;
	space newspace;
	
	lseek(*spacefile, 0, SEEK_SET);
	lseek(newspacefile, 0, SEEK_SET);
			
	while (lastused--) {
	
		towrite = *used - lastwritten;

		while (towrite > BUFFERED_BLOCKS) {

                	if (read(*spacefile, buf, 2 * SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        	pdie("Read failed");

                	if (write(newspacefile, buf, 2 * SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        	pdie("Write failed");

                	towrite -= BUFFERED_BLOCKS;
        	}

        	if (read(*spacefile, buf, 2 * SKEYRECORDSIZE * (towrite + 1)) < 0)
                	pdie("Read failed");

        	if (write(newspacefile, buf, 2 * SKEYRECORDSIZE * towrite) < 0)
                	pdie("Write failed");
	
		archive2skey(&(newspace.begin), buf + towrite * 2 * SKEYRECORDSIZE, 1);
		newspace.end = *skey;
		
		if (newspace.end > (newspace.begin + BLOCKSIZE)) {
		
			skey2archive(&(newspace.begin), skeyarchive, 1);
			skey2archive(&(newspace.end), skeyarchive + SKEYRECORDSIZE, 1);

			if (write(newspacefile, skeyarchive, 2 * SKEYRECORDSIZE) < 0)
                		pdie("Write failed");
		
		}
		
		newspace.begin = *skey + blocksize;
		archive2skey(&(newspace.end), buf + towrite * 2 * SKEYRECORDSIZE + SKEYRECORDSIZE, 1);

		if (newspace.end > (newspace.begin + BLOCKSIZE)) {
	
			skey2archive(&(newspace.begin), skeyarchive, 1);
			skey2archive(&(newspace.end), skeyarchive + SKEYRECORDSIZE, 1);

			if (write(newspacefile, skeyarchive, 2 * SKEYRECORDSIZE) < 0)
                		pdie("Write failed");
		
		}
		
		lastwritten = *used + 1;
		
		*used++;
		*skey++;
	}

	int readbytes = 0;

	if ((readbytes = read(*spacefile, buf, 2 * SKEYRECORDSIZE * BUFFERED_BLOCKS)) < 0)
              	pdie("Read failed");

	while (readbytes) {

               	if (write(newspacefile, buf, readbytes) < 0)
                      	pdie("Write failed");

		if ((readbytes = read(*spacefile, buf, 2 * SKEYRECORDSIZE * BUFFERED_BLOCKS)) < 0)
        	      	pdie("Read failed");
	
       	}


	*spacefile = newspacefile;

	free(buf);
	free(skeyarchive);
	
}

u_int64_t get_skey_in_space(space *activespace, int blocksize)
{

        uint subrnd = 0;
        int flag = 0;
        u_int64_t result = 0;

        uint max = activespace->end - activespace->begin; /*potential error uint*/

        while (!flag) {

                subrnd = rand() % max;

                if (activespace->end > (activespace->begin + subrnd + blocksize)) {

                        result = activespace->begin + subrnd;
                        flag = 1;

                } else {

                        max = subrnd;

                }
        }

        return result;

}

int simple_search(uint key, uint *array, int maxindex)
{

	int i;
	for (i = 0; i < maxindex; i++)
		if (array[i] == key)
			return 1;
	
	return 0;

}

void make_skey(int *spacefile, u_int64_t *skey, uint keysize, int blocksize)
{

        uint rnd = 0;
        uint completed = 0;
        int level = 0;
        
        uint spaces = ffile_size(*spacefile) / (SKEYRECORDSIZE * 2);
 
        if (spaces == 0) {

        	printf("Input is too big for this device\nExiting.\n");
                exit(1);

        }

	uint *used = (uint *) calloc(keysize, sizeof(uint));
	int i;
	for (i = 0; i < keysize; i++)
		*(used + i) = (uint) -1;

	int lastused = 0;
		
	u_int64_t *sortedskey = (u_int64_t *) calloc(keysize, 8);

	space newspace = {0, 0};

        while (completed < keysize) {

                rnd = rand() % spaces;

                if (simple_search(rnd, used, lastused + 1) == 0) {

			read_space(*spacefile, rnd, &newspace);
			
			if (newspace.end > (newspace.begin + blocksize)) {
			
                        	*(skey + lastused) = get_skey_in_space(&newspace, blocksize);
                        	*(used + lastused) = rnd;

                        	lastused++;                        
                        	completed++;
                        	level = 0;
                        
                        	spaces--;
                        }

                }  else {

                        level += 1;

                }
                
                if ((spaces == 0) || (level == MAXLEVEL)) {
                
                	qsort(used, lastused, sizeof(uint), uint_cmp);
                	
                	memcpy(sortedskey, skey, lastused * 8); /**/
                	qsort(sortedskey, lastused, 8, u_int64_t_cmp);			
			
			rewrite_space(spacefile, used, sortedskey, lastused, blocksize);
			
			skey += lastused;			

			spaces = ffile_size(*spacefile) / (SKEYRECORDSIZE * 2);
			
			if (spaces == 0) {

        			printf("@Input is too big for this device\nExiting.\n");
                		exit(1);

        		}
			
			for (i = 0; i < lastused; i++)
				*(used + i) = (uint) -1;
	
			lastused = 0;
                
                }

        }
        
        if (lastused != 0) {
        
        	qsort(used, lastused, sizeof(uint), uint_cmp);
        	
		memcpy(sortedskey, skey, lastused * 8); /**/	
		qsort(sortedskey, lastused, 8, u_int64_t_cmp);
		
		rewrite_space(spacefile, used, sortedskey, lastused, blocksize);
	
	}
	
	free(used);
	free(sortedskey);
}

void make_skey_main(int *spacefile, int skeyfile, uint keysize)
{

        uint i = 0;
        u_int64_t *skey   = (u_int64_t *) calloc(BUFFERED_BLOCKS, 8);
        uchar 	  *buffer = (uchar *) 	  calloc(BUFFERED_BLOCKS * SKEYRECORDSIZE, 1);

        while (keysize > BUFFERED_BLOCKS) {

                make_skey(spacefile, skey, BUFFERED_BLOCKS, BLOCKSIZE);
                skey2archive(skey, buffer, BUFFERED_BLOCKS);

                if (write(skeyfile, buffer, BUFFERED_BLOCKS * SKEYRECORDSIZE) < 0)
                        pdie("Write failed");

                keysize -= BUFFERED_BLOCKS;

        }

        make_skey(spacefile, skey, keysize, BLOCKSIZE);
        skey2archive(skey, buffer, keysize);

        if (write(skeyfile, buffer, keysize * SKEYRECORDSIZE) < 0)
                pdie("Write failed");

        free(buffer);
        free(skey);

}

void write_data(device *activedev, uchar *block, int length, u_int64_t address)
{

        extern int errno;
        int writtenchars;

        lseek(activedev->descriptor, address, SEEK_SET);
        
        int offset = 0;

        while (length != 0) {
                if ((writtenchars = write(activedev->descriptor, block + offset, length)) < 0)
                        if (errno = EINTR)
                                continue;
                        else
                                pdie("Write failed");

                length -= writtenchars;
                offset += writtenchars;
        }
}

void read_data(device *activedev, uchar *block, uint length, u_int64_t address)
{

        extern int errno;

        lseek(activedev->descriptor, address, SEEK_SET);

        int ret = 0;
        int offset = 0;

        while (length != 0) {
                if ((ret = read(activedev->descriptor, block + offset, length)) < 0)
                        if (errno == EINTR)
                                continue;
                        else
                                pdie("Read failed");

                length -= ret;
                offset += ret;

        }

}

void write_by_skey(device *activedev, int datafile, int skeyfile, uint blocks)
{

        u_int64_t *skey   = (u_int64_t *) calloc(BUFFERED_BLOCKS, 8);
        uchar     *buffer = (uchar *)     calloc(BUFFERED_BLOCKS * SKEYRECORDSIZE, 1);
        uchar     *block  = (uchar *)     calloc(BLOCKSIZE * BUFFERED_BLOCKS, 1);

        int i;

        while (blocks > BUFFERED_BLOCKS) {

                if (read(skeyfile, buffer, SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Read failed");

                archive2skey(skey, buffer, BUFFERED_BLOCKS);

                if (read(datafile, block, BLOCKSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Read failed");

                for (i = 0; i < BUFFERED_BLOCKS; i++)
                        write_data(activedev, block + i * BLOCKSIZE, BLOCKSIZE, *(skey + i));

                blocks -= BUFFERED_BLOCKS;
        }

        if (read(skeyfile, buffer, SKEYRECORDSIZE * blocks) < 0)
                pdie("Read failed");

        archive2skey(skey, buffer, blocks);

        if (read(datafile, block, BLOCKSIZE * blocks) < 0)
                pdie("Read failed");

        for (i = 0; i < blocks; i++)
                write_data(activedev, block + i * BLOCKSIZE, BLOCKSIZE, *(skey + i));

        free(skey);
        free(buffer);
        free(block);

}

void get_data(device *activedev, int datafile, u_int64_t skeyaddress, uint keysize)
{

        u_int64_t *skey   = (u_int64_t *) calloc(BUFFERED_BLOCKS, 8);
        uchar     *buffer = (uchar *)     calloc(BUFFERED_BLOCKS * SKEYRECORDSIZE, 1);
        uchar     *block  = (uchar *)     calloc(BLOCKSIZE * BUFFERED_BLOCKS, 1);

        int i;

        while (keysize > BUFFERED_BLOCKS) {

                read_data(activedev, buffer, BUFFERED_BLOCKS * SKEYRECORDSIZE, skeyaddress);

                archive2skey(skey, buffer, BUFFERED_BLOCKS);

                for (i = 0; i < BUFFERED_BLOCKS; i++)
                        read_data(activedev, block + i * BLOCKSIZE, BLOCKSIZE, *(skey + i));

                if (write(datafile, block, BLOCKSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Write failed");

                skeyaddress += BUFFERED_BLOCKS * SKEYRECORDSIZE;
                keysize -= BUFFERED_BLOCKS;
        }

        read_data(activedev, buffer, keysize * SKEYRECORDSIZE, skeyaddress);

        archive2skey(skey, buffer, keysize);

        for (i = 0; i < keysize; i++)
                read_data(activedev, block + i * BLOCKSIZE, BLOCKSIZE, *(skey + i));

        if (write(datafile, block, BLOCKSIZE * keysize) < 0)
                pdie("Write failed");

        free(skey);
        free(buffer);
        free(block);

}

u_int64_t buffer_for_skey(int *spacefile, uint skeylen)
{

        u_int64_t result = 0;

        make_skey(spacefile, &result, 1, skeylen * SKEYRECORDSIZE);

        return result;

}

void write_skey(device *activedev, int skeyfile, uint skeylen, u_int64_t address)
{

        lseek(activedev->descriptor, address, SEEK_SET);
        lseek(skeyfile, 0, SEEK_SET);

        uchar *block = (uchar *) calloc(SKEYRECORDSIZE * BUFFERED_BLOCKS, 1);

        int readchars = 0;

        while (skeylen > BUFFERED_BLOCKS) {

                if (read(skeyfile, block, SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Read failed");

                if (write(activedev->descriptor, block, SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        pdie("Write failed");

                skeylen -= BUFFERED_BLOCKS;
        }

        if (read(skeyfile, block, SKEYRECORDSIZE * skeylen) < 0)
                pdie("Read failed");

        if (write(activedev->descriptor, block, SKEYRECORDSIZE * skeylen) < 0)
                pdie("Write failed");

        free(block);

}

u_int64_t device_size(device *activedev)
{

        u_int64_t size = 0;

        if(ioctl(activedev->descriptor, BLKGETSIZE64, &size) != -1)
                ;
        else
                pdie("Getting device size failed");

        return size;

}

void immer_main(int mode, char *devicename, names filename, char *charpassword, config conf)
{

	extern int space1fd;
	extern int space2fd;

        device activedev;

        int datafile;
        int keyfile;

        int outdatafile;
        int outkeyfile;

        int dataskeyfile;
        int keyskeyfile;
        
        int spacefile;

        struct pass password = {0, 0, 0};

        uint skeysize = 0;
        u_int64_t datalen = 0;

        printf("\nOpening device...");

        if (conf.isfile)
                activedev.descriptor = open(devicename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        else
                activedev.descriptor = open(devicename, O_RDWR);

        if (activedev.descriptor < 0)
                pdie("Device open failed. Maybe wrong device selected?");

        printf(".done\n");

        printf("Opening files...");

        if (mode == ENCRYPT) {

                datafile = open(filename.enc, O_RDONLY);
                keyfile  = open(filename.key, O_RDONLY);

                dataskeyfile = open(filename.dataskey, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                keyskeyfile  = open(filename.keyskey, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		
		space1fd = open("space.1", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		space2fd = open("space.2", O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                spacefile = space1fd;
                if (datafile < 0 || keyfile < 0 || dataskeyfile < 0 || keyskeyfile < 0 || spacefile < 0)
                        pdie("Open failed");

        }

        if (mode == DECRYPT) {

                outdatafile = open(filename.data, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
                outkeyfile  = open(filename.key,  O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

                if (outdatafile < 0 || outkeyfile < 0)
                        pdie("Open failed");

        }

        printf(".done\n");

        u_int64_t dataskeyaddress = 0;
        u_int64_t keyskeyaddress = 0;

        if (mode == ENCRYPT) {

                printf("Getting device size...");
                if (conf.isfile)
                        activedev.size = conf.filesize;
                else
                        activedev.size = device_size(&activedev);
                        
                printf(".done %lld\n", activedev.size);

                printf("Calculating length of input...");
                datalen = file_size(filename.data);
                printf(".done %lld\n", datalen);

                printf("Calculating size of skey...");
                skeysize = datalen/BLOCKSIZE;
                printf(".done %u\n", skeysize);
                
                uchar *firstspace = (uchar *) calloc(SKEYRECORDSIZE * 2, 1);
                u_int64_t zero = 0;
                skey2archive(&(zero), firstspace, 1);
		skey2archive(&(activedev.size), firstspace + SKEYRECORDSIZE, 1);
		
		if (write(spacefile, firstspace, 2 * SKEYRECORDSIZE) < 0)
                	pdie("Write failed");
		free(firstspace);

                printf("Setting buffers for data skey and key skey...\n");
                dataskeyaddress = buffer_for_skey(&spacefile, skeysize);
                keyskeyaddress  = buffer_for_skey(&spacefile, skeysize);
		
                printf("Making skey for data...\n");
                make_skey_main(&spacefile, dataskeyfile, skeysize);

                printf("Making skey for key...\n");
                make_skey_main(&spacefile, keyskeyfile, skeysize);

                printf("Filling device with random data...\n");
                fill_random(&activedev, activedev.size);

                lseek(dataskeyfile, 0, SEEK_SET);
                lseek(keyskeyfile, 0, SEEK_SET);

                printf("Writing data by skey...\n");
                write_by_skey(&activedev, datafile, dataskeyfile, skeysize);

                printf("Writing key by skey...\n");
                write_by_skey(&activedev, keyfile, keyskeyfile, skeysize);

                printf("Writing skey's...\n");
                write_skey(&activedev, dataskeyfile, skeysize, dataskeyaddress);
                write_skey(&activedev, keyskeyfile, skeysize, keyskeyaddress);

                uchar *outpassword = (uchar *) calloc(19, 1); /**/
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
                get_data(&activedev, outdatafile, password.dataskeyaddress, skeysize);
                printf(".done\n");

                printf("Getting key by skey...");
                get_data(&activedev, outkeyfile, password.keyskeyaddress, skeysize);
                printf(".done\n");
        }

        close(activedev.descriptor);
        if (mode == ENCRYPT) {
        	close(spacefile);
                close(datafile);
                close(keyfile);
                
                close(space1fd);
                close(space2fd);

                close(dataskeyfile);
                close(keyskeyfile);
                
                close(space1fd);
                close(space2fd);
        }

        if (mode == DECRYPT) {
                close(outdatafile);
                close(outkeyfile);
        }

}

