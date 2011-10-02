/*
 * Immersion theory library.
 *
 * Copyright (C) 2010 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

 /* Globals. Need fix */
 int space1fd;
 int space2fd;

void gamma_cipher(uchar *out, uchar *in, uchar *key, int len)
{
        while(len--)
                *out++ = *in++ ^ *key++;
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

void write_data(int destination_file, uchar *block, off_t length, off_t address)
{

        extern int errno;

        lseek(destination_file, address, SEEK_SET);

        trusted_write(destination_file, block, length);
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

void write_skey(struct device *dev, int skeyfile, uint skeylen, off_t address)
{

        lseek(dev->descriptor, address, SEEK_SET);
        lseek(skeyfile, 0, SEEK_SET);

	copy(skeyfile, dev->descriptor, skeylen * SKEYRECORDSIZE);
}

struct space {

        off_t begin;
        off_t end;

};

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
		
		copy(*spacefile, newspacefile, towrite * 2 * SKEYRECORDSIZE);
		
		if (read(*spacefile, buffer, 2 * SKEYRECORDSIZE) < 0)
                	pdie("Read failed");		
		
		archive2skey(buffer, &(newspace.begin), 1);
		newspace.end = *skey;
		
		if (newspace.end > (newspace.begin + BLOCKSIZE)) {
		
			skey2archive(&(newspace.begin), skeyarchive, 1);
			skey2archive(&(newspace.end), skeyarchive + SKEYRECORDSIZE, 1);

			trusted_write(newspacefile, skeyarchive, 2 * SKEYRECORDSIZE);
		}
		
		newspace.begin = *skey + blocksize;
		archive2skey(buffer + SKEYRECORDSIZE, &(newspace.end), 1);
		
		if (newspace.end > (newspace.begin + BLOCKSIZE)) {
	
			skey2archive(&(newspace.begin), skeyarchive, 1);
			skey2archive(&(newspace.end), skeyarchive + SKEYRECORDSIZE, 1);
			
			trusted_write(newspacefile, skeyarchive, 2 * SKEYRECORDSIZE);
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

uint calc_spaces(int spacefile)
{

	uint spaces = ffile_size(spacefile) / (SKEYRECORDSIZE * 2);
 
        if (spaces == 0) {

        	printf("Input is too big for this device\nExiting.\n");
                exit(1);
        }
        
	return spaces;
}

void make_skey(int *spacefile, off_t *skey, uint keysize, int blocksize)
{
        
        uint spaces = calc_spaces(*spacefile);

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

			spaces = calc_spaces(*spacefile);
			
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

off_t buffer_for_skey(int *spacefile, uint skeylen)
{

        off_t result = 0;

        make_skey(spacefile, &result, 1, skeylen * SKEYRECORDSIZE);

        return result;

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
                        write_data(dev->descriptor, block + i * BLOCKSIZE,
                        				BLOCKSIZE, *(skey + i));

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
