/*
 * Immersion library.
 *
 * Copyright (C) 2010 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

void pdie(const char *mesg) {
   perror(mesg);
   exit(1);
}

int is_free(int device, unsigned long long int adress, unsigned int length, unsigned int skip) {

    unsigned char *buffer = (unsigned char *) malloc(length);
    
    int flag=0;
    int i=0;
    
    lseek(device,adress+skip,SEEK_SET);
    
    if (read(device,buffer,length)>0) {
    
        for (i=0;i<length;i++) {
    
            if (buffer[i]=='\0') { /*NULL*/
        
                flag=1;
        
            } else {

                flag=0;
                break;
        
            }
        }
    } else 
        pdie("Read failed");
        
    
    return flag;
    
    free(buffer);
    

}

void fill_random(int device, unsigned int blocks, unsigned int skip, int type) { 

    lseek(device, skip, SEEK_SET);
    
    if (type==URANDOM) {
    
        int urandom= open("/dev/urandom",O_RDONLY);
    
        if (urandom<0)
            pdie("Opening pseudorandom numbers generator failed");
    
        unsigned char *buffer = (unsigned char *) malloc(BLOCKSIZE);
    
        while (blocks--) {
        
            if (read(urandom, buffer, BLOCKSIZE)<0)
                pdie("Read failed");
            
            if (write(device, buffer, BLOCKSIZE)<0)
                pdie("Write failed");
    
        }
    }
    
    if (type==SRAND) {
        int i=0;
        unsigned char *randomblock=malloc(BLOCKSIZE);
        
        while (blocks--) {
            for(i=0;i<BLOCKSIZE;i++)
                *(randomblock+i)=rand()%256;
                
            if (write(device, randomblock, BLOCKSIZE));
        }
        
        free(randomblock);
    }

}

void fill_zero(int device, unsigned int blocks, unsigned int skip) {

    lseek(device,skip,SEEK_SET);
    unsigned char *zeros=(unsigned char *) malloc(BLOCKSIZE);
    memset(zeros,0x00,BLOCKSIZE);
    
    while (blocks--) {
    
        if (write(device,zeros,BLOCKSIZE)<0)
            pdie("Write failed");
    
    }
    
    free(zeros);

}

void write_data(int device, void *block, unsigned int length, unsigned long long int address, unsigned int skip) {

    lseek(device,address+skip,SEEK_SET);
    if (write(device,block,length)<0)
        pdie("Write failed");

}

void read_data(int device, void *block, unsigned int length, unsigned long long int address, unsigned int skip) {

    lseek(device,address+skip,SEEK_SET);
    if (read(device,block,length)<0)
        pdie("Read failed");

}

void set_flags(int device, unsigned long long int address, unsigned int length, unsigned int skip) {

    unsigned char *flags=(unsigned char *) malloc(length);
    memset(flags,0x80,length);
    
    lseek(device,address+skip,SEEK_SET);
    if (write(device,flags,length)<0)
        pdie("Write failed");
    
    free(flags);
    
}

void skey2archive(unsigned long long int *skey, unsigned char *buffer, unsigned int skeysize) {

    int i=0;
    for (i=0;i<skeysize;i++) {
    
        memcpy(buffer+i*SKEYRECORDSIZE,skey+i,SKEYRECORDSIZE);
    
    }

}

void archive2skey(unsigned long long int *skey, unsigned char *buffer, unsigned int skeysize) {

    int i=0;
    for (i=0;i<skeysize;i++) {
    
        memcpy(skey+i,buffer+i*SKEYRECORDSIZE,SKEYRECORDSIZE);
    
    }

}

void make_skey(int device, unsigned long long int devicesize, unsigned long long int *skey, unsigned int keysize, int blocksize, unsigned int skip) {

    unsigned long long int rnd=0;
    unsigned int completed=0;
    int level=0;
    
    while (completed<keysize) {
        
        rnd=rand()%devicesize;
        if (rnd<devicesize-blocksize) 
            if (is_free(device, rnd, blocksize, skip)) {
            
                *skey++=rnd;
                completed++;
                set_flags(device,rnd,blocksize,skip);
                level=0;
            
            } else if (is_free(device, rnd-blocksize, blocksize,skip)) {
            
                *skey++=rnd-blocksize;
                completed++;
                set_flags(device,rnd-blocksize,blocksize,skip);
                level=0;
            
            } else {
            
                level+=1;
            
            }  

        if (level==MAXLEVEL) {
        
            printf("Input is too big for this device\nExiting.\n");
            exit(0);
        
        }
    
    }
    

}

void make_skey_main(int device, unsigned long long int devicesize, int skeyfile, unsigned int keysize, unsigned int skip) {
    
    unsigned int i=0; 
    unsigned long long int skey=0;
    unsigned char *buffer = calloc(1, SKEYRECORDSIZE);

    for (i=0;i<keysize;i++) {
    
        make_skey(device, devicesize, &skey, 1, BLOCKSIZE, skip);
        skey2archive(&skey,buffer,1);
        write(skeyfile,buffer,SKEYRECORDSIZE);

    }
    
    free(buffer);

}

void write_by_skey(int device, int datafile, int skeyfile, unsigned int blocks, unsigned int skip) {

    unsigned long long int skey=0;
    unsigned char *buffer = calloc(1, SKEYRECORDSIZE);
    
    unsigned char *block = calloc(BLOCKSIZE,1);
    
    int buffercount=0;
    int i=0;
    
    for (i=0;i<blocks;i++) {

        read(skeyfile, buffer, SKEYRECORDSIZE);
        archive2skey(&skey, buffer, 1);

        read(datafile, block, BLOCKSIZE);
        write_data(device,block,BLOCKSIZE,skey,skip);

    }
    
    free(buffer);
    free(block);

}

void get_data(int device, int datafile, unsigned long long int skeyaddress, unsigned int skeysize, unsigned int skip) {

    unsigned long long int skey=0;
    unsigned char *buffer = calloc(1, SKEYRECORDSIZE);
    
    unsigned char *block = calloc(BLOCKSIZE,1);
    
    int i=0;
    
    for(i=0;i<skeysize;i++) {
    
        
        read_data(device, buffer, SKEYRECORDSIZE, skeyaddress+i*SKEYRECORDSIZE, skip);
        archive2skey(&skey, buffer, 1);

        read_data(device, block,BLOCKSIZE,skey,skip);
        write(datafile, block, BLOCKSIZE);    
    }
    
    free(buffer);
    free(block);
    
}

unsigned long long int file_size(unsigned char *filename) {

    struct stat filestat;
    stat(filename,&filestat);
    
    return filestat.st_size;

}

unsigned long long int buffer_for_skey(int device, unsigned long long int devicesize, unsigned int skeylen, unsigned int skip) {

    unsigned long long int result=0;

    make_skey(device, devicesize, &result, 1, skeylen*SKEYRECORDSIZE,skip);    
    
    return result;

}

void write_skey(int device, int skeyfile, unsigned int skeylen, unsigned long long int address, unsigned int skip) {

    lseek(device,address+skip,SEEK_SET);
    lseek(skeyfile,0,SEEK_SET);
    
    unsigned char *block=calloc(BLOCKSIZE,1);
    
    int readchars=0;
    
    while (skeylen-=BLOCKSIZE > 0) {
        if ((readchars=read(skeyfile,block,BLOCKSIZE))<0)
            pdie("Read failed");
        
        if (write(device,block,readchars)<0)
            pdie("Write failed");
        
    } 
    
    free(block);   

}

unsigned long long int device_size(int device) {

    unsigned long long size=0;

    #ifdef DEV
    if(ioctl(device,BLKGETSIZE64,&size)!=-1)
        ;
    else
        pdie("Getting device size failed");
    #endif
    
    #ifndef DEV
    size=1024*1024*64;
    #endif
    
    return size;

}

unsigned long long int set_skip(int fd) { /**/

    return 512;

}

void make_mbr() {}
void write_boot_partition() {}


void immer_main(int mode, char *devicename, char *datafilename, char *keyfilename, char *charpassword, char *dataskeyfilename, char *keyskeyfilename) {

    int device;
    
    int datafile;
    int keyfile;
    
    int outdatafile;
    int outkeyfile;
    
    int dataskeyfile;
    int keyskeyfile;
    
    struct pass password = {0,0,0}; 
    
    unsigned int skeysize=0;
    unsigned long long int devicesize=0;
    unsigned long long int datalen=0;
    unsigned int skip=0;
    
    printf("Opening device...");
    device = open(devicename,O_RDWR);
    if (device<0)
        pdie("Device open failed. Maybe wrong device selected?");
        
    printf(".done\n");
    
    /*Open files*/
    printf("\nOpening files...");
    
    if (mode==ENCRYPT) {

        datafile = open(datafilename,O_RDONLY);
        keyfile  = open(keyfilename,O_RDONLY);
        
        dataskeyfile = open(dataskeyfilename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        keyskeyfile  = open(keyskeyfilename, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        
        if (datafile <0 || keyfile <0 || dataskeyfile <0 || keyskeyfile<0)
            pdie("Open failed");
        
    }
    
    if (mode==DECRYPT) {
        
        outdatafile = open(datafilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        outkeyfile  = open(keyfilename, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);

        if (outdatafile <0 || outkeyfile <0)
            pdie("Open failed");

    }
    
    printf(".done\n");
    /*End of Open files*/
    
    printf("Getting device size...");
    devicesize = device_size(device); 
    printf(".done %lld\n", devicesize);  
    
    printf("Setting skip for mbr and boot partition...");
    skip = set_skip(device);
    printf(".done %u\n", skip);  
    
    
    printf("Initialising random number numbers generator...");
    srand(time(NULL));
    printf(".done\n");
    
    unsigned long long int dataskeyaddress=0;
    unsigned long long int keyskeyaddress=0;  
    
    devicesize-=skip;
    
    if (mode==ENCRYPT) {
    
        printf("Calculating length of input...");
        datalen=file_size(datafilename);
        printf(".done %lld\n", datalen);
        
        printf("Calculating size of skey...");
        skeysize=datalen/BLOCKSIZE;
        printf(".done %u\n", skeysize);
    
        printf("Preparing device. All data will be destroyed...");
        fill_zero(device,devicesize/BLOCKSIZE,skip);
        puts(".done\n");
        
        printf("Setting buffers for data skey and key skey...");
        dataskeyaddress = buffer_for_skey(device, devicesize, skeysize,skip);
        keyskeyaddress  = buffer_for_skey(device, devicesize, skeysize,skip);
        printf(".done\n");
        
        printf("Making skey for data...");
        make_skey_main(device, devicesize, dataskeyfile, skeysize, skip);
        printf(".done\n");
        
        printf("Making skey for key...");
        make_skey_main(device, devicesize, keyskeyfile, skeysize, skip);
        printf(".done\n");    
        
        printf("Filling device with random data...");
        fill_random(device,devicesize/BLOCKSIZE,skip, SRAND);
        printf(".done\n");
        
        lseek(dataskeyfile, 0, SEEK_SET);
        lseek(keyskeyfile, 0, SEEK_SET);
        
        printf("Writing data by skey...");
        write_by_skey(device,datafile,dataskeyfile,skeysize,skip);
        printf(".done\n");
        
        printf("Writing key by skey...");
        write_by_skey(device,keyfile,keyskeyfile,skeysize,skip);
        printf(".done\n");
        
        printf("Writing skey's...");
        write_skey(device,dataskeyfile,skeysize*SKEYRECORDSIZE, dataskeyaddress,skip);
        write_skey(device,keyskeyfile,skeysize*SKEYRECORDSIZE, keyskeyaddress,skip);
        printf(".done\n");
        
        unsigned char *outpassword = calloc(20,1);
        make_password(outpassword, skeysize, dataskeyaddress, keyskeyaddress);
        
        printf("Your's password: %s\n", outpassword);
        
        free(outpassword);
    }
    
    if (mode==DECRYPT) {
    
        printf("Preparing password...");
        prepare_password(charpassword, &password);
        printf(".done\n");
        
        printf("Getting size of skey...");
        skeysize=password.skeysize;
        printf(".done %u\n", skeysize);

        printf("Getting data by skey...");
        get_data(device,outdatafile,password.dataskeyaddress,skeysize,skip);
        printf(".done\n");
        
        printf("Getting key by skey...");   
        get_data(device,outkeyfile,password.keyskeyaddress,skeysize,skip);   
        printf(".done\n");
    }
   
    
    /*Close files*/

    close(device);
    if (mode==ENCRYPT) {
        close(datafile);
        close(keyfile);
        
        close(dataskeyfile);
        close(keyskeyfile);
    }
    
    if (mode==DECRYPT) {
        close(outdatafile);
        close(outkeyfile);
    }
    /*End of close files*/

}

