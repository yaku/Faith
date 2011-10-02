/*
 * Faith cryptosystem. Static archive.
 *
 * Copyright (C) 2010 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

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

                fill_random(key_file, file_size(filename.data));

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
 
void immer_main(int mode, char *devicename, names filename, uchar *charpassword, 
								    config conf)
{

	extern int space1fd;
	extern int space2fd;

        printf("Opening device...\n");
        struct device dev;
        
        if (conf.isfile)
                dev.descriptor = open(devicename, O_RDWR | O_CREAT, 
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
		
		if (conf.fillrandom == 1) {
                	printf("Filling device with random data...\n");
                	fill_random(dev.descriptor, dev.size);
		}
		
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
