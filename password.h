/*
 * Functions for making and transforming password.
 *
 * Copyright (C) 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */


struct pass {
    
        uint skeysize;
        u_int64_t dataskeyaddress;
        u_int64_t keyskeyaddress;
    
} pass;

void stage2pass(u_int64_t stage, unsigned char *pass, int sixbitsequences) 
{

        unsigned char table[] = "/1234567890abcdefghijklmnopqrstuvwxyz+ABCDEFGHIJKLMNOPQRSTUVWXYZ";    
    
        int i = 0;
    
        for (i = 0; i < sixbitsequences; i++)
                *(pass + sixbitsequences - 1 - i) = table[(((stage >> 6 * i) << (64 - 6))  >> (64 - 6))];

}

u_int64_t pass2stage( unsigned char *pass, int sixbitsequences) 
{

        unsigned char table[] = "/1234567890abcdefghijklmnopqrstuvwxyz+ABCDEFGHIJKLMNOPQRSTUVWXYZ";

        u_int64_t stage = 0;
        int i = 0;
    
        for (i = 0; i < sixbitsequences; i++) {
                stage += ((unsigned char *) memchr(table, pass[i], 64) - table);
                stage <<= 6;
        }    
        stage >>= 6;
    
        return stage;
}

u_int64_t arr2long(unsigned char *arr, int bytes) 
{

        u_int64_t stage = 0;
        int i = 0;    

    
        for (i = 0; i < bytes; i++) {
                stage += arr[i];
                stage <<= 8;
        }
        stage >>= 8;
    
        return stage;

}

void long2arr(u_int64_t stage, unsigned char *arr, int bytes) 
{

        int i = 0;     
    
        for (i = 0; i < bytes; i++)
                *(arr + bytes - 1 - i) = (((stage >> 8 * i) << (64 - 8))  >> (64 - 8));

}

void arr2pass(unsigned char *in, unsigned char *pass) 
{

        u_int64_t stage1 = 0;
        u_int64_t stage2 = 0;
        u_int64_t stage3 = 0;

        stage1 = arr2long(in, 6);
        stage2 = arr2long(in + 6, 6);
        stage3 = arr2long(in + 12, 2);
    
        stage2pass(stage1, pass, 8);
        stage2pass(stage2, pass + 8, 8);
        stage2pass(stage3, pass + 16, 3);
    
}

void pass2arr(unsigned char *pass, unsigned char *arr) 
{

        u_int64_t stage1 = 0;
        u_int64_t stage2 = 0;
        u_int64_t stage3 = 0;
    
        stage1=pass2stage(pass, 8);
        stage2=pass2stage(pass + 8, 8);
        stage3=pass2stage(pass + 16, 3);
    
        long2arr(stage1, arr, 6);
        long2arr(stage2, arr + 6, 6);
        long2arr(stage3, arr + 12, 2);
}

void replace_raw2mixed(unsigned char *raw, unsigned char *mixed) 
{

        int i = 0;
        
        for (i = 0; i < 14; i += 2)
                *mixed++ = *(raw + i);
        
        for (i = 1; i < 14; i += 2)
                *mixed++ = *(raw + i);
                
}

void replace_mixed2raw(unsigned char *mixed, unsigned char *raw) 
{

        int i = 0;
        
        for (i = 0; i < 14; i += 2)
                *(raw + i) = *mixed++;
        
        for (i = 1; i < 14; i += 2)
                *(raw + i) = *mixed++;
                
}

void prepare_password(unsigned char *charpassword, struct pass *password) 
{
    
        unsigned char *rawarr = calloc(15, 1);
        unsigned char *mixedarr = calloc(15, 1);
        
        pass2arr(charpassword, mixedarr);
        
        replace_mixed2raw(mixedarr ,rawarr);
    
        password->keyskeyaddress  = arr2long(rawarr, 5);
        password->dataskeyaddress = arr2long(rawarr + 5, 5);
        password->skeysize = (uint) arr2long(rawarr + 10, 4);
    
        free(rawarr);
        free(mixedarr);
    
}

void make_password(unsigned char *outpassword, uint skeysize, u_int64_t dataskeyaddress, u_int64_t keyskeyaddress) 
{

        unsigned char *rawarr = calloc(15, 1);
        unsigned char *mixedarr = calloc(15, 1);
    
        long2arr(keyskeyaddress, rawarr, 5);
        long2arr(dataskeyaddress, rawarr + 5, 5);
        long2arr((u_int64_t) skeysize, rawarr + 10, 4);
        
        replace_raw2mixed(rawarr, mixedarr);
    
        arr2pass(mixedarr, outpassword);
        
        free(mixedarr);
        free(rawarr);
}
