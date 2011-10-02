/*
 * IO functions.
 *
 * Copyright (C) 2010, 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

struct device {

        int descriptor;
        off_t size;

};

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

off_t device_size(struct device *dev)
{

        off_t size = 0;

        if(ioctl(dev->descriptor, BLKGETSIZE64, &size) < 0)
                pdie("Getting device size failed");

        return size;

}
