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

        unsigned char buffer[BUFFER_SIZE];   /* Read/Write buffer. */
        unsigned char keybuffer[BUFFER_SIZE];
        unsigned char bigbuffer[BBUFFER_SIZE];
        unsigned char keybbuffer[BBUFFER_SIZE];
    
        unsigned char *bp;   /* Pointer into write buffer. */
        unsigned char *bbp;
        unsigned char *kbp; 
        unsigned char *kbbp; 
        unsigned char *userkey;
        unsigned char *tmpbuf;
    
        int writtenChars;  /* Number of bytes written on last write. */
        int bigbufferChars; /* Number of bytes remaining to be written. */
        int keybufferChars;
        int tmp;    
    
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
    
        userkey = (unsigned char*) malloc(512);
        tmpbuf = (unsigned char *) malloc(BUFFER_SIZE);
    

    
        if (mode==ENCRYPT) {
                kbbp=keybbuffer;
                /*Main file cipher*/
                while (1) {
                /* some number of bytes read. */
                        if ((bigbufferChars = read (rfd, bigbuffer, BBUFFER_SIZE)) > 0) {
                                bbp = bigbuffer;
                                tmp = bigbufferChars;

                                while (bigbufferChars > 0) {
                                        bp = buffer;
                                        memcpy(bp, bbp, BUFFER_SIZE);
                    
                                        if (bigbufferChars < BUFFER_SIZE) 
                                                while (bigbufferChars < BUFFER_SIZE) 
                                                        buffer[bigbufferChars++]=0;

                    
                                        bigbufferChars-=BUFFER_SIZE;
                    
                    
                                        make_rand_userkey(userkey, 512);
                                        memcpy(kbbp,userkey, 512);
                                        kbbp += 512;
                                        memcpy(tmpbuf, bp, 512);
                    
                                        gamma_cipher(bp, tmpbuf, userkey, 512);
                    
                                        memcpy(bbp, bp, BUFFER_SIZE);
                    
                                        bbp += BUFFER_SIZE;
                                }
                                bigbufferChars = tmp;
                                bbp = bigbuffer;
                                kbbp = keybbuffer;
                                write(kfd, kbbp, bigbufferChars);
                                while (bigbufferChars > 0) {
                                        if ((writtenChars = write(wfd, bbp, bigbufferChars) ) < 0)
                                                pdie("Write failed");
        
                                        bigbufferChars -= writtenChars; 
                                        bbp += writtenChars;
                                }
    
                        } else if (bigbufferChars == 0)   /* EOF reached. */
                                break;
                        else
                                pdie("Read failed");
            
        }
        /*End of Main file cipher*/
        } else {
    
                /*Main file decipher*/
                while (1) {
                /* some number of bytes read. */
                        if ((bigbufferChars = read(rfd, bigbuffer, BBUFFER_SIZE)) > 0 && (keybufferChars = read(kfd, keybbuffer, BBUFFER_SIZE)) >0 ) {
            
                                if (bigbufferChars != keybufferChars) 
                                        pdie("Invalid keyfile");
                                        
                                bbp = bigbuffer;
                                kbbp = keybbuffer;
                                tmp = bigbufferChars;
                
                                while (bigbufferChars > 0) {
                                        bp = buffer;
                                        kbp = keybuffer;
                                        memcpy(bp, bbp, BUFFER_SIZE);
                                        memcpy(kbp, kbbp, BUFFER_SIZE);
                    
                                        if (bigbufferChars < BUFFER_SIZE) 
                                                while (bigbufferChars < BUFFER_SIZE) {
                                                        keybuffer[bigbufferChars] = 0;
                                                        buffer[bigbufferChars++] = 0;
                            
                                                }
                            
                                        bigbufferChars -= BUFFER_SIZE;
                    
                    
                                        gamma_cipher(tmpbuf, bp, kbp, 512);
                    
                                        memcpy(bbp, tmpbuf, BUFFER_SIZE);
                    
                                        bbp += BUFFER_SIZE;
                                        kbbp += BUFFER_SIZE;
                                }
                                
                                bigbufferChars=tmp;
                                bbp=bigbuffer;
                                kbp=keybuffer;
                                write(kfd, kbp, bigbufferChars);
                                while (bigbufferChars > 0) {
                                        if ((writtenChars = write(wfd, bbp, bigbufferChars) ) < 0)
                                                pdie("Write failed");
        
                                        bigbufferChars -= writtenChars; 
                                        bbp += writtenChars;
                                }
    
                        } else if (bigbufferChars == 0)   /* EOF reached. */
                                break;
                        else
                                pdie("Read failed");
            
                }
        /*End of Main file decipher*/
    
        }

        /*Close files*/
        close(rfd);
        close(wfd);
        close(kfd);
        /*End of Close files*/


        return 0;
}
