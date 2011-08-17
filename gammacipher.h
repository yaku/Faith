/*
 * Gamma xoring library and cipher.
 *
 * Copyright (C) 2010 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

void gamma_cipher(uchar *out, uchar *in, uchar *key, int len)
{
        while(len--)
                *out++ = *in++ ^ *key++;
}

void gammacipher_main (int mode, names filename)
{

        int rfd;
        int kfd;
        int wfd;

        int written;
        int readchars;
        int keyreadchars;

        if ((rfd = open(filename.data, O_RDONLY)) < 0)
                pdie("Open failed");

        if ((wfd = open(filename.enc, O_WRONLY | O_CREAT | O_TRUNC, 
				      S_IWUSR | S_IRUSR)) < 0)
                pdie("Open failed");

        if (mode == ENCRYPT)
                if ((kfd = open(filename.key, O_RDWR | O_CREAT | O_TRUNC, 
					      S_IWUSR | S_IRUSR)) < 0)
                        pdie("Open failed");

        if (mode == DECRYPT)
                if ((kfd = open(filename.key, O_RDONLY)) < 0)
                        pdie("Open failed");

        uchar *userkey    = malloc(BUFFER_SIZE);
        uchar *bigbuffer  = malloc(BUFFER_SIZE);
        uchar *ciphertext = malloc(BUFFER_SIZE);

        if (mode == ENCRYPT) {

                struct device keyfile = {kfd, file_size(filename.data)};
                fill_random(&keyfile, keyfile.size);

                lseek(kfd, 0, SEEK_SET);
        }

        while ((readchars = read(rfd, bigbuffer, BUFFER_SIZE)) && 
	       (keyreadchars = read(kfd, userkey, BUFFER_SIZE))) {
		    
		if ((readchars < 0) || (keyreadchars < 0))
			pdie("Read failed");

		/*
		 * possibly paranoia
		 * need fix
		 */
        	if (readchars != keyreadchars)
        	        pdie("Invalid keyfile");

        	gamma_cipher(ciphertext, bigbuffer, userkey, readchars);

        	while (readchars) {
                
                	if ((written = write(wfd, ciphertext, readchars)) < 0)
        	        	pdie("Write failed");
                    	    
                    	readchars -= written;
                }
        } 

        free(userkey);
        free(bigbuffer);
        free(ciphertext);

        close(rfd);
        close(wfd);
        close(kfd);
}
