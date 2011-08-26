/*
 * Immersion library.
 *
 * Copyright (C) 2010, 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */
 
 /* Globals. Need fix */
 int space1fd;
 int space2fd;

struct device {

        int descriptor;
        off_t size;

};

struct space {

        off_t begin;
        off_t end;

};

off_t file_size(char *filename)
{

        struct stat filestat;
        stat(filename, &filestat);

        return filestat.st_size;

}

off_t ffile_size(int fd)
{

        struct stat filestat;
        fstat(fd, &filestat);

        return filestat.st_size;

}

void trusted_write(int destination_file, uchar *buffer, off_t bytes_to_write)
{

	off_t written_bytes;

	while (bytes_to_write) {
                
               	if ((written_bytes = write(destination_file, buffer, 
               					       	   bytes_to_write)) < 0)
                 		pdie("Write failed");
                    	    
                bytes_to_write -= written_bytes;
                buffer += written_bytes;
        }
}

void copy(int source_file, int destination_file, off_t nbytes)
{

        uchar *buffer = malloc(BUFFER_SIZE);
	int bytes_read;
	
        while (nbytes) {

	        if ((bytes_read = read(source_file, buffer, 
	        	    			 MIN(nbytes, BUFFER_SIZE))) < 0)
	                pdie("Read failed");
		
		nbytes -= bytes_read;
		
                trusted_write(destination_file, buffer, bytes_read);
	}

        free(buffer);
}

void fill_random(int file, off_t nbytes)
{

        lseek(file, 0, SEEK_SET);

        int urandom = open("/dev/urandom", O_RDONLY);

        if (urandom < 0)
	        pdie("Opening pseudorandom numbers generator failed");
	
	copy(urandom, file, nbytes);
}

void archive2skey(uchar *buffer, off_t *skey, uint skeysize)
{

        int i = 0;
        for (i = 0; i < skeysize; i++)
                memcpy(skey + i, buffer + i * SKEYRECORDSIZE, SKEYRECORDSIZE);
}

void skey2archive(off_t *skey, uchar *buffer, uint skeysize)
{

        int i = 0;
        for (i = 0; i < skeysize; i++)
                memcpy(buffer + i * SKEYRECORDSIZE, skey + i, SKEYRECORDSIZE);
}

int off_t_cmp(const void *long1, const void *long2)
{
	
	if (*((off_t *) long1) > *((off_t *) long2))
		return 1;

	else if (*((off_t *) long1) == *((off_t *) long1))
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

void read_space(int spacefile, uint index, struct space *outspace)
{

	uchar *buffer = calloc(SKEYRECORDSIZE * 2, 1);
	
	if (pread(spacefile, buffer, SKEYRECORDSIZE * 2, 
						index * SKEYRECORDSIZE * 2) < 0)
		pdie("Read failed");

	outspace->begin = 0;
	outspace->end = 0;
	
	archive2skey(buffer, &(outspace->begin), 1);
	archive2skey(buffer + SKEYRECORDSIZE, &(outspace->end), 1);
	
	free(buffer);
}

void rewrite_space(int *spacefile, uint *used, off_t *skey, int lastused, 
								int blocksize)
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

	uchar *buffer = calloc(2 * SKEYRECORDSIZE * BUFFERED_BLOCKS, 1); 
	uchar *skeyarchive = calloc(2 * SKEYRECORDSIZE, 1);
	int lastwritten_bytes = 0;
	int towrite 	= 0;
	struct space newspace;
	
	lseek(*spacefile, 0, SEEK_SET);
	lseek(newspacefile, 0, SEEK_SET);
			
	while (lastused--) {
	
		towrite = *used - lastwritten_bytes;

		while (towrite > BUFFERED_BLOCKS) {

                	if (read(*spacefile, buffer,
                		      2 * SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        	pdie("Read failed");

                	if (write(newspacefile, buffer,
                		      2 * SKEYRECORDSIZE * BUFFERED_BLOCKS) < 0)
                        	pdie("Write failed");

                	towrite -= BUFFERED_BLOCKS;
        	}

        	if (read(*spacefile, buffer, 2 * SKEYRECORDSIZE * (towrite + 1)) < 0)
                	pdie("Read failed");

        	if (write(newspacefile, buffer, 2 * SKEYRECORDSIZE * towrite) < 0)
                	pdie("Write failed");
	
		archive2skey(buffer + towrite * 2 * SKEYRECORDSIZE, &(newspace.begin), 1);
		newspace.end = *skey;
		
		if (newspace.end > (newspace.begin + BLOCKSIZE)) {
		
			skey2archive(&(newspace.begin), skeyarchive, 1);
			skey2archive(&(newspace.end), skeyarchive + SKEYRECORDSIZE, 1);

			if (write(newspacefile, skeyarchive, 2 * SKEYRECORDSIZE) < 0)
                		pdie("Write failed");
		
		}
		
		newspace.begin = *skey + blocksize;
		archive2skey(buffer + towrite * 2 * SKEYRECORDSIZE + SKEYRECORDSIZE, &(newspace.end), 1);

		if (newspace.end > (newspace.begin + BLOCKSIZE)) {
	
			skey2archive(&(newspace.begin), skeyarchive, 1);
			skey2archive(&(newspace.end), skeyarchive + SKEYRECORDSIZE, 1);

			if (write(newspacefile, skeyarchive, 2 * SKEYRECORDSIZE) < 0)
                		pdie("Write failed");
		
		}
		
		lastwritten_bytes = *used + 1;
		
		used++;
		skey++;
	}

	int readbytes;

	while ((readbytes = read(*spacefile, buffer,
					2 * SKEYRECORDSIZE * BUFFERED_BLOCKS))) {
	
		if (readbytes < 0)
			pdie("Read failed");
		
		trusted_write(newspacefile, buffer, readbytes);
       	}


	*spacefile = newspacefile;

	free(buffer);
	free(skeyarchive);
}

off_t get_skey_in_space(struct space *activespace, int blocksize)
{

        uint random_space_offset = 0;
        int found = 0;
        off_t result = 0;

        uint max_offset = activespace->end - activespace->begin; /*potential error uint*/

        while (!found) {

                random_space_offset = rand() % max_offset;

                if (activespace->end > 
                       (activespace->begin + random_space_offset + blocksize)) {

                        result = activespace->begin + random_space_offset;
                        found = 1;

                } else {

                        max_offset = random_space_offset;

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

void make_skey(int *spacefile, off_t *skey, uint keysize, int blocksize)
{
        
        uint spaces = ffile_size(*spacefile) / (SKEYRECORDSIZE * 2);
 
        if (spaces == 0) {

        	printf("Input is too big for this device\nExiting.\n");
                exit(1);

        }

	uint *usedids = calloc(keysize, sizeof(uint));
	int i;
	for (i = 0; i < keysize; i++)
		*(usedids + i) = (uint) -1;

	int lastused = 0;
		
	off_t *sortedskey = calloc(keysize, 8);

	struct space newspace = {0, 0};
	
	uint newspaceid = 0;
        uint completed = 0;
        int level = 0;

        while (completed < keysize) {

                newspaceid = rand() % spaces;

                if (simple_search(newspaceid, usedids, lastused + 1) == 0) {

			read_space(*spacefile, newspaceid, &newspace);
			
			if (newspace.end > (newspace.begin + blocksize)) {
			
                        	*(skey + lastused) = get_skey_in_space(&newspace,
                        					     blocksize);
                        	*(usedids + lastused) = newspaceid;

                        	lastused++;                        
                        	completed++;
                        	level = 0;
                        
                        	spaces--;
                        }

                }  else {

                        level += 1;
                }
                
                if ((spaces == 0) || (level == MAXLEVEL)) {
                
                	qsort(usedids, lastused, sizeof(uint), uint_cmp);
                	
                	memcpy(sortedskey, skey, lastused * 8); /**/
                	qsort(sortedskey, lastused, 8, off_t_cmp);			
			
			rewrite_space(spacefile, usedids, sortedskey, lastused, 
								     blocksize);
			
			skey += lastused;

			spaces = ffile_size(*spacefile) / (SKEYRECORDSIZE * 2);
			
			if (spaces == 0) {

        			printf("@Input is too big for this device\n"
        			       "Exiting.\n");
                		exit(1);

        		}
			
			for (i = 0; i < lastused; i++)
				*(usedids + i) = (uint) -1;
	
			lastused = 0;
                
                }

        }
        
        if (lastused != 0) {
        
        	qsort(usedids, lastused, sizeof(uint), uint_cmp);
        	
		memcpy(sortedskey, skey, lastused * 8); /**/	
		qsort(sortedskey, lastused, 8, off_t_cmp);
		
		rewrite_space(spacefile, usedids, sortedskey, lastused, blocksize);
	
	}
	
	free(usedids);
	free(sortedskey);
}

void make_skey_main(int *spacefile, int skeyfile, uint keysize)
{

        off_t *skey = calloc(BUFFERED_BLOCKS, 8);
        uchar *buffer = calloc(BUFFERED_BLOCKS * SKEYRECORDSIZE, 1);
	
	int to_make;
	
        while (keysize) {
		
		to_make = MIN(keysize, BUFFERED_BLOCKS);
		
                make_skey(spacefile, skey, to_make, BLOCKSIZE);
                skey2archive(skey, buffer, to_make);

		trusted_write(skeyfile, buffer, to_make * SKEYRECORDSIZE);
		
                keysize -= to_make;
        }

        free(buffer);
        free(skey);
}

void write_data(struct device *dev, uchar *block, off_t length, off_t address)
{

        extern int errno;

        lseek(dev->descriptor, address, SEEK_SET);

        trusted_write(dev->descriptor, block, length);
}

void read_data(struct device *dev, uchar *block, uint length, off_t address)
{

        extern int errno;

        lseek(dev->descriptor, address, SEEK_SET);

        int bytes_read = 0;

        while (length) {
        
                if ((bytes_read = read(dev->descriptor, block, length)) < 0) {
                
                        if (errno == EINTR)
                                continue;
                        else
                                pdie("Read failed");
                }

                length -= bytes_read;
                block += bytes_read;
        }
}

void write_by_skey(struct device *dev, int datafile, int skeyfile, uint blocks)
{

        off_t *skey = calloc(BUFFERED_BLOCKS, 8);
        uchar *buffer = calloc(BUFFERED_BLOCKS * SKEYRECORDSIZE, 1);
        uchar *block  = calloc(BLOCKSIZE * BUFFERED_BLOCKS, 1);

        int i, to_write;

        while (blocks) {
        
        	to_write = MIN(blocks, BUFFERED_BLOCKS);

                if (read(skeyfile, buffer, SKEYRECORDSIZE * to_write) < 0)
                        pdie("Read failed");

                archive2skey(buffer, skey, to_write);

                if (read(datafile, block, BLOCKSIZE * to_write) < 0)
                        pdie("Read failed");

                for (i = 0; i < to_write; i++)
                        write_data(dev, block + i * BLOCKSIZE, BLOCKSIZE,
                        					   *(skey + i));

                blocks -= to_write;
        }

        free(skey);
        free(buffer);
        free(block);

}

void get_data(struct device *dev, int datafile, off_t skeyaddress, uint keysize)
{

        off_t *skey   = calloc(BUFFERED_BLOCKS, 8);
        uchar *buffer = calloc(BUFFERED_BLOCKS * SKEYRECORDSIZE, 1);
        uchar *block  = calloc(BLOCKSIZE * BUFFERED_BLOCKS, 1);

        int i, to_read;

        while (keysize) {
		
		to_read = MIN(keysize, BUFFERED_BLOCKS);
		
                read_data(dev, buffer, to_read * SKEYRECORDSIZE, skeyaddress);

                archive2skey(buffer, skey, to_read);

                for (i = 0; i < to_read; i++)
                        read_data(dev, block + i * BLOCKSIZE, BLOCKSIZE, *(skey + i));

                if (write(datafile, block, BLOCKSIZE * to_read) < 0)
                        pdie("Write failed");

                skeyaddress += to_read * SKEYRECORDSIZE;
                keysize -= to_read;
        }

        free(skey);
        free(buffer);
        free(block);

}

off_t buffer_for_skey(int *spacefile, uint skeylen)
{

        off_t result = 0;

        make_skey(spacefile, &result, 1, skeylen * SKEYRECORDSIZE);

        return result;

}

void write_skey(struct device *dev, int skeyfile, uint skeylen, off_t address)
{

        lseek(dev->descriptor, address, SEEK_SET);
        lseek(skeyfile, 0, SEEK_SET);

	copy(skeyfile, dev->descriptor, skeylen * SKEYRECORDSIZE);
}

off_t device_size(struct device *dev)
{

        off_t size = 0;

        if(ioctl(dev->descriptor, BLKGETSIZE64, &size) < 0)
                pdie("Getting device size failed");

        return size;

}

void immer_main(int mode, char *devicename, names filename, uchar *charpassword, 
								    config conf)
{

	extern int space1fd;
	extern int space2fd;

        printf("Opening device...\n");
        struct device dev;
        
        if (conf.isfile)
                dev.descriptor = open(devicename, O_RDWR | O_CREAT | O_TRUNC, 
                					     S_IRUSR | S_IWUSR);
        else
                dev.descriptor = open(devicename, O_RDWR);

        if (dev.descriptor < 0)
                pdie("Device open failed. Maybe wrong device selected?");

        printf("Opening files...\n");
        int datafile;
        int keyfile;
        int dataskeyfile;
        int keyskeyfile;
        int spacefile;

        if (mode == ENCRYPT) {

                datafile = open(filename.enc, O_RDONLY);
                keyfile  = open(filename.key, O_RDONLY);

                dataskeyfile = open(filename.dataskey, O_RDWR | O_CREAT | O_TRUNC,
                				       	     S_IRUSR | S_IWUSR);
                keyskeyfile  = open(filename.keyskey, O_RDWR | O_CREAT | O_TRUNC, 
                				             S_IRUSR | S_IWUSR);
		
		space1fd = open("space.1", O_RDWR | O_CREAT | O_TRUNC, 
					   S_IRUSR | S_IWUSR);
		space2fd = open("space.2", O_RDWR | O_CREAT | O_TRUNC, 
					   S_IRUSR | S_IWUSR);
                spacefile = space1fd;
                if (datafile < 0 || keyfile < 0 || dataskeyfile < 0 || 
                                               keyskeyfile < 0 || spacefile < 0)
                        pdie("Open failed");

        }
        
        int outdatafile;
        int outkeyfile;

        if (mode == DECRYPT) {

                outdatafile = open(filename.data, O_WRONLY | O_CREAT | O_TRUNC, 
                					     S_IRUSR | S_IWUSR);
                outkeyfile  = open(filename.key,  O_WRONLY | O_CREAT | O_TRUNC, 
                					     S_IRUSR | S_IWUSR);

                if (outdatafile < 0 || outkeyfile < 0)
                        pdie("Open failed");

        }

        off_t dataskeyaddress = 0;
        off_t keyskeyaddress = 0;
	
        uint skeysize = 0;
        off_t datalen = 0;
        
        if (mode == ENCRYPT) {

                printf("Getting device size...");
                if (conf.isfile)
                        dev.size = conf.filesize;
                else
                        dev.size = device_size(&dev);
                        
                printf(".done %lld\n", dev.size);

                printf("Calculating length of input...");
                datalen = file_size(filename.data);
                printf(".done %lld\n", datalen);

                printf("Calculating size of skey...");
                skeysize = datalen/BLOCKSIZE;
                printf(".done %u\n", skeysize);
                
                uchar *firstspace = calloc(SKEYRECORDSIZE * 2, 1);
                off_t zero = 0;
                skey2archive(&(zero), firstspace, 1);
		skey2archive(&(dev.size), firstspace + SKEYRECORDSIZE, 1);
		
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
                fill_random(dev.descriptor, dev.size);

                lseek(dataskeyfile, 0, SEEK_SET);
                lseek(keyskeyfile, 0, SEEK_SET);

                printf("Writing data by skey...\n");
                write_by_skey(&dev, datafile, dataskeyfile, skeysize);

                printf("Writing key by skey...\n");
                write_by_skey(&dev, keyfile, keyskeyfile, skeysize);

                printf("Writing skey's...\n");
                write_skey(&dev, dataskeyfile, skeysize, dataskeyaddress);
                write_skey(&dev, keyskeyfile, skeysize, keyskeyaddress);

                uchar *outpassword = calloc(19, 1); /**/
                make_password(outpassword, skeysize, dataskeyaddress, keyskeyaddress);

                printf("Your's password: %s\n", outpassword);

                free(outpassword);
        }
        

        if (mode == DECRYPT) {
        	
	        struct pass password = {0, 0, 0};

                prepare_password(charpassword, &password);

                printf("Getting size of skey...");
                skeysize = password.skeysize;
                printf(".done %u\n", skeysize);

                printf("Getting data by skey.\n");
                get_data(&dev, outdatafile, password.dataskeyaddress, skeysize);

                printf("Getting key by skey.\n");
                get_data(&dev, outkeyfile, password.keyskeyaddress, skeysize);
                
        }

        close(dev.descriptor);
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
