/*
 * Gamma xoring library and cipher.
 *
 * Copyright (C) 2010 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

void die(const char *mesg) 
{
        fputs(mesg, stderr);
        fputc('\n', stderr);
        exit(1);
}

void gamma_cipher(unsigned char *out, unsigned char *in, unsigned char *key, int len) 
{

        int i;
        for (i = 0; i < len; i++)
                *out++ = *in++ ^ *key++; 

}

int gammacipher_main (int mode, char *datafilename, char *keyfilename, char *outfilename, config conf) 
{

        int rfd;   /* Read file descriptor. */
        int kfd;   /* Key file descriptor. */
        int wfd;   /* Write file descriptor. */
    
        int writtenchars;  /* Number of bytes written on last write. */
        int readchars; /* Number of bytes remaining to be written. */
        int keyreadchars;
        
        if ((rfd = open(datafilename, O_RDONLY, 0)) < 0) 
                pdie("Open failed"); /* Open input file*/
                
        if ((wfd = open(outfilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) 
                pdie("Open failed"); /* Open output file*/   
    
        if (mode == ENCRYPT)
                if ((kfd = open(keyfilename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) 
                        pdie("Open failed");   

        if (mode == DECRYPT)
                if ((kfd = open(keyfilename, O_RDONLY, 0)) < 0) 
                        pdie("Open failed");       
    
        unsigned char *userkey = malloc(BUFFER_SIZE);
        unsigned char *bigbuffer = malloc(BUFFER_SIZE);
        unsigned char *ciphertext = malloc(BUFFER_SIZE);
    
        if (mode==ENCRYPT) {
                
                device keyfile = {kfd, file_size(datafilename), 0}; 
                fill_random(&keyfile, keyfile.size, conf.randomizer);
                
                lseek(kfd, 0, SEEK_SET);
                
        } 
        
        while (1) {
                
                if ((readchars = read(rfd, bigbuffer, BUFFER_SIZE)) > 0 && (keyreadchars = read(kfd, userkey, BUFFER_SIZE)) > 0) {
                
                        if (readchars != keyreadchars) 
                                pdie("Invalid keyfile");
                
                        gamma_cipher(ciphertext, bigbuffer, userkey, readchars);
                
                        if ((writtenchars = write(wfd, ciphertext, readchars) ) < 0)
                                pdie("Write failed");
                                
                        if (writtenchars != readchars)
                                printf("IO warning\n");
                        
                        
                } else if (readchars == 0) 
                        break;
                else
                        pdie("Read failed");
        }
        
        free(userkey);
        free(bigbuffer);
        free(ciphertext);

        close(rfd);
        close(wfd);
        close(kfd);
        
        return 0;
}
