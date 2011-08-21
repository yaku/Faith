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

        int input_file = -1;
        int key_file = -1;
        int output_file = -1;

        if ((input_file = open(filename.data, O_RDONLY)) < 0)
                pdie("Open failed");

        if ((output_file = open(filename.enc, O_WRONLY | O_CREAT | O_TRUNC, 
				      			S_IWUSR | S_IRUSR)) < 0)
                pdie("Open failed");

        if (mode == ENCRYPT)
                if ((key_file = open(filename.key, O_RDWR | O_CREAT | O_TRUNC, 
					      		S_IWUSR | S_IRUSR)) < 0)
                        pdie("Open failed");

        if (mode == DECRYPT)
                if ((key_file = open(filename.key, O_RDONLY)) < 0)
                        pdie("Open failed");

        if (mode == ENCRYPT) {

                struct device keyfile = {key_file, file_size(filename.data)};
                fill_random(&keyfile, keyfile.size);

                lseek(key_file, 0, SEEK_SET);
        }
        
        uchar *key_buffer = malloc(BUFFER_SIZE);
        uchar *input_buffer = malloc(BUFFER_SIZE);
        uchar *output_buffer = malloc(BUFFER_SIZE);
	
        int written_bytes;
        int bytes_read;
        int keybytes_read;

        while ((bytes_read = read(input_file, input_buffer, BUFFER_SIZE)) && 
	       (keybytes_read = read(key_file, key_buffer, BUFFER_SIZE))) {
		    
		if ((bytes_read < 0) || (keybytes_read < 0))
			pdie("Read failed");

		/*
		 * possibly paranoia
		 * need fix
		 */
        	if (bytes_read != keybytes_read)
        	        pdie("Invalid keyfile");

        	gamma_cipher(output_buffer, input_buffer, key_buffer, 
        							    bytes_read);

        	while (bytes_read) {
                
                	if ((written_bytes = write(output_file, output_buffer,
                					       bytes_read)) < 0)
        	        	pdie("Write failed");
                    	    
                    	bytes_read -= written_bytes;
                }
        } 

        free(key_buffer);
        free(input_buffer);
        free(output_buffer);

        close(input_file);
        close(output_file);
        close(key_file);
}
