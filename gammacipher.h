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

void make_rand_userkey(unsigned char *userkey, int keylen) 
{
        while (keylen--)
                *userkey++ = rand() % 256;
}

void gamma_cipher(unsigned char *out, unsigned char *in, unsigned char *key, int len) 
{

        int i;
        for (i = 0; i < len; i++)
                *out++ = *in++ ^ *key++; 

}

int gammacipher_main (int mode, char *datafilename, char *keyfilename, char *outfilename) 
{

        int rfd;   /* Read file descriptor. */
        int kfd;   /* Key file descriptor. */
        int wfd;   /* Write file descriptor. */
    
        int writtenchars;  /* Number of bytes written on last write. */
        int readchars; /* Number of bytes remaining to be written. */
        int keyreadchars;
    
        if (mode == ENCRYPT) {
    
                if ((rfd = open(datafilename, O_RDONLY, 0)) < 0) 
                        pdie("Open failed"); /* Open input file*/
        
                if ((kfd = open(keyfilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) 
                        pdie("Open failed");
            
                if ((wfd = open(outfilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) 
                        pdie("Open failed"); /* Open output file*/    
    
        }
    
        if (mode == DECRYPT) {
    
                if ((rfd = open(datafilename, O_RDONLY, 0)) < 0) 
                        pdie("Open failed"); /* Open input file*/
        
                if ((kfd = open(keyfilename, O_RDONLY, 0)) < 0) 
                        pdie("Open failed");       
        
                if ((wfd = open(outfilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) < 0) 
                        pdie("Open failed"); /* Open output file*/       
    
        }

        /*End of Open files*/
    
    
        srand(time(NULL)); /*Init randomizer*/
    
        unsigned char *userkey = malloc(BUFFER_SIZE);
    
        unsigned char *bigbuffer = malloc(BUFFER_SIZE);
        unsigned char *ciphertext = malloc(BUFFER_SIZE);
        
        int tmp;
    
        if (mode==ENCRYPT) {
                /*Main file cipher*/
                
                while (1) {
                
                        if ((readchars = read (rfd, bigbuffer, BUFFER_SIZE)) > 0) {
                
                                make_rand_userkey(userkey, readchars);
                        
                                gamma_cipher(ciphertext, bigbuffer, userkey, readchars);
                        
                                if ((writtenchars = write(wfd, ciphertext, readchars) ) < 0)
                                        pdie("Write failed");
                                
                                if (writtenchars != readchars) /*need fix*/
                                        printf("IO warning\n");
                                
                                if ((writtenchars = write(kfd, userkey, readchars) ) < 0)
                                        pdie("Write failed");
                                        
                                if (writtenchars != readchars)
                                        printf("IO warning\n");
                                
                        } else if (readchars == 0) 
                                break;
                        else
                                pdie("Read failed");
                
                }
        } else {
    
                /*Main file decipher*/
                while (1) {
                /* some number of bytes read. */
                
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
    
        }
        
        free(userkey);
        free(bigbuffer);
        free(ciphertext);

        /*Close files*/
        close(rfd);
        close(wfd);
        close(kfd);
        /*End of Close files*/


        return 0;
}
