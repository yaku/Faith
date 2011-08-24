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
        off_t dataskeyaddress;
        off_t keyskeyaddress;
    
} pass;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void base64_encode(uchar *in, size_t inlen, uchar *out, size_t outlen)
{
	
        int i = 0;
        
        uchar table[] = "/1234567890abcdefghijklmnopqrstu"
        		"vwxyz+ABCDEFGHIJKLMNOPQRSTUVWXYZ";    
	
	size_t input_bytes;
	size_t current_len = 0;
	u_int64_t buff = 0;
	int out_bytes = 0;
	
	while (inlen) {
	
		buff = 0;
		input_bytes = MIN(inlen, 3);
		
		memcpy(&buff, in, input_bytes);
		out_bytes = MIN((int) floor(input_bytes * 4 / 3.0), 
							(outlen - current_len));
		
        	for (i = 0; i < out_bytes + 1; i++)
                	*(out + i) = table[(((buff >> 6 * i) << (64 - 6))  >> (64 - 6))];
		
		in += input_bytes;
		out += out_bytes;
		inlen -= input_bytes;
		current_len += out_bytes;
	}
}

void base64_decode(uchar *in, size_t inlen, uchar *out, size_t outlen)
{

	uchar table[] = "/1234567890abcdefghijklmnopqrstu"
        		"vwxyz+ABCDEFGHIJKLMNOPQRSTUVWXYZ"; 

        uint buff = 0;
        size_t current_len = 0;
        int i = 0;
    	
    	uint index = 0;
    	uint index_with_bitshift = 0;
    	size_t input_bytes = 0;
    	int out_bytes = 0;
    	
    	while (inlen) {
    		
    		input_bytes = MIN(inlen, 4);
		
		out_bytes = MIN((int) floor(input_bytes * 3.0 / 4), 
							(outlen - current_len));
        	
    		buff = 0;
        	
        	for (i = 0; i < input_bytes; i++) {
        	
        		index = (uint) ((uchar *) memchr(table, *(in + i), 64) - table);
        	        index_with_bitshift = (index << (6 * i));
        	        buff |= index_with_bitshift;
        	}
        	
        	memcpy(out, &buff, out_bytes);
        	
        	in += input_bytes;
		out += out_bytes;
		inlen -= input_bytes;
		current_len += out_bytes;
	}
}

void replace_raw2mixed(uchar *raw, uchar *mixed) 
{

        int i = 0;
        
        for (i = 0; i < 14; i += 2)
                *mixed++ = *(raw + i);
        
        for (i = 1; i < 14; i += 2)
                *mixed++ = *(raw + i);
                
}

void replace_mixed2raw(uchar *mixed, uchar *raw) 
{

        int i = 0;
        
        for (i = 0; i < 14; i += 2)
                *(raw + i) = *mixed++;
        
        for (i = 1; i < 14; i += 2)
                *(raw + i) = *mixed++;
                
}

void prepare_password(uchar *charpassword, struct pass *password) 
{
    
        uchar *rawarr = calloc(15, 1);
        uchar *mixedarr = calloc(15, 1);
        
        base64_decode(charpassword, strlen((char *) charpassword), mixedarr, 15);
        
        replace_mixed2raw(mixedarr ,rawarr);
        
        memcpy(&(password->keyskeyaddress), rawarr, 5);
        memcpy(&(password->dataskeyaddress), rawarr + 5, 5);
    	memcpy(&(password->skeysize), rawarr + 10, 4);
      
        free(rawarr);
        free(mixedarr);
    
}

void make_password(uchar *outpassword, uint skeysize, off_t dataskeyaddress,
						      off_t keyskeyaddress) 
{

        uchar *rawarr = calloc(15, 1);
        uchar *mixedarr = calloc(15, 1);

	memcpy(rawarr, &keyskeyaddress, 5);
	memcpy(rawarr + 5, &dataskeyaddress, 5);
	memcpy(rawarr + 10, &skeysize, 4);
	
        replace_raw2mixed(rawarr, mixedarr);
    
        base64_encode(mixedarr, 14, outpassword, 19);
        
        free(mixedarr);
        free(rawarr);
}
