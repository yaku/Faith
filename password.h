/*
 * Functions for making and transforming password.
 *
 * Copyright (C) 2010 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

struct pass {
    
        unsigned int skeysize;
        unsigned long long int dataskeyaddress;
        unsigned long long int keyskeyaddress;
    
} pass;

void stage2pass(unsigned long long int stage, unsigned char *pass, int sixbitsequences) 
{

        unsigned char table[] = "1234567890@+qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";    
    
        int i = 0;
    
        for (i = 0; i < sixbitsequences; i++)
                *(pass + sixbitsequences - 1 - i) = table[(((stage >> 6 * i) << (64 - 6))  >> (64 - 6))];

}

unsigned long long int pass2stage( unsigned char *pass, int sixbitsequences) 
{

        unsigned char table[] = "1234567890@+qwertyuiopasdfghjklzxcvbnmQWERTYUIOPASDFGHJKLZXCVBNM";

        unsigned long long int stage = 0;
        int i = 0;
    
        for (i = 0; i < sixbitsequences; i++) {
                stage += ((unsigned char *) memchr(table, pass[i], 64) - table);
                stage <<= 6;
        }    
        stage >>= 6;
    
        return stage;
}

unsigned long long int arr2long(unsigned char *arr, int bytes) 
{

        unsigned long long int stage = 0;
        int i = 0;    

    
        for (i = 0; i < bytes; i++) {
                stage += arr[i];
                stage <<= 8;
        }
        stage >>= 8;
    
        return stage;

}

void long2arr(unsigned long long int stage, unsigned char *arr, int bytes) 
{

        int i = 0;     
    
        for (i = 0; i < bytes; i++)
                *(arr + bytes - 1 - i) = (((stage >> 8 * i) << (64 - 8))  >> (64 - 8));

}

void arr2pass(unsigned char *in, unsigned char *pass) 
{

        unsigned long long int stage1 = 0;
        unsigned long long int stage2 = 0;
        unsigned long long int stage3 = 0;

        stage1 = arr2long(in, 6);
        stage2 = arr2long(in + 6, 6);
        stage3 = arr2long(in + 12, 2);
    
        stage2pass(stage1, pass, 8);
        stage2pass(stage2, pass + 8, 8);
        stage2pass(stage3, pass + 16, 3);
    
}

void pass2arr(unsigned char *pass, unsigned char *arr) 
{

        unsigned long long int stage1 = 0;
        unsigned long long int stage2 = 0;
        unsigned long long int stage3 = 0;
    
        stage1=pass2stage(pass, 8);
        stage2=pass2stage(pass + 8, 8);
        stage3=pass2stage(pass + 16, 3);
    
        long2arr(stage1, arr, 6);
        long2arr(stage2, arr + 6, 6);
        long2arr(stage3, arr + 12, 2);
}

void prepare_password(unsigned char *charpassword, struct pass *password) 
{
    
        unsigned char *rawarr = calloc(15, 1);
        int i = 0;
        pass2arr(charpassword, rawarr);
    
        password->keyskeyaddress  = arr2long(rawarr, 5);
        password->dataskeyaddress = arr2long(rawarr + 5, 5);
        password->skeysize = (unsigned int) arr2long(rawarr + 10, 4);
    
        free(rawarr);
    
}

void make_password(unsigned char *outpassword, unsigned int skeysize, unsigned long long int dataskeyaddress, unsigned long long int keyskeyaddress) 
{

        unsigned char *rawarr = calloc(15, 1);
        int i = 0;
    
        long2arr(keyskeyaddress, rawarr, 5);
        long2arr(dataskeyaddress, rawarr + 5, 5);
        long2arr((unsigned long long int) skeysize, rawarr + 10, 4);
    
        arr2pass(rawarr, outpassword);
    
        free(rawarr);
}
