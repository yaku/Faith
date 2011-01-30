/*
 * Faith Crypto. Immersion theory cipher.
 * Copyright (C) 2010 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "defines.h"
#include "includes.h"

#include "die.h"
#include "config.h"
#include "password.h"
#include "immer.h"
#include "gammacipher.h"
#include "filename.h"

void print_usage() 
{
        printf("Usage: faith -[e,d] device [-f filelist] [-p pass]\n"); 
        exit(0);
}

int main(int argc, char **argv) 
{

        int mode=0;
        
        /*Check comand line arguments*/
        if (argc == 1) 
                print_usage();
        
        if (strcmp(argv[1], "-d") == 0)
                mode = DECRYPT;
                
        else if (strcmp(argv[1], "-e") == 0)
                mode = ENCRYPT;
                
        else
                print_usage();
   
        if (mode == ENCRYPT && strcmp(argv[3], "-f") != 0) 
                print_usage();
        
        if (mode == DECRYPT && strcmp(argv[3], "-p") != 0) 
                print_usage();  
        
        if (argc != 5)
                print_usage();
        /*End of check comand line arguments*/
            
        char *datafilename      = malloc(FILENAMEMAXLENGTH);
        char *keyfilename       = malloc(FILENAMEMAXLENGTH);
        char *encfilename       = malloc(FILENAMEMAXLENGTH); 
        char *dataskeyfilename  = malloc(FILENAMEMAXLENGTH); 
        char *keyskeyfilename   = malloc(FILENAMEMAXLENGTH); 
    
        char *command = malloc(1024);
    
    
        if (mode == ENCRYPT) {
    
                printf("Encryption started\n");
        
                printf("Making names...");
                make_time_filename(datafilename);
                make_filename(KEYFILE, datafilename, keyfilename);
                make_filename(ENCFILE, datafilename, encfilename);
                make_filename(DATASKEYFILE, datafilename, dataskeyfilename);
                make_filename(KEYSKEYFILE, datafilename, keyskeyfilename);
                printf(".done\n");
        
                printf("Running cpio. Making archive...");
                snprintf(command, 1024, "cpio -o -O %s < %s", datafilename, argv[4]);
                if (system(command) != 0)
                        pdie("Cpio error");
                printf(".done\n");
        
                printf("Main gamma cipher encryption started...");
                gammacipher_main(mode, datafilename, keyfilename, encfilename);
                printf(".done\n");
        
                printf("Dive data to device started...");
                immer_main(mode, argv[2], encfilename, keyfilename, 0, dataskeyfilename, keyskeyfilename);
                printf(".done\n");
        
                printf("Cleaning work directory. Removing files...");
                unlink(keyfilename);
                unlink(encfilename);
                unlink(datafilename);
                unlink(dataskeyfilename);
                unlink(keyskeyfilename);
                printf(".done\n");
        }

        if (mode == DECRYPT) {
    
                printf("Decryption started\n");
    
                printf("Making names...");
                make_out_time_filename(DATA, datafilename);
                make_out_time_filename(KEY, keyfilename);
                make_filename(ENCFILE, datafilename, encfilename);
                printf(".done\n");
        
                printf("Re-diving data from device...");
                immer_main(mode, argv[2], encfilename, keyfilename, (unsigned char *) argv[4], 0, 0);
                printf(".done\n");
        
                printf("Main gamma cipher decryption started...");
                gammacipher_main(mode, encfilename, keyfilename, datafilename);
                printf(".done\n");
        
                printf("Running cpio. Extractiong archive...");
                snprintf(command, 1024, "cpio -i -I %s", datafilename);
                if (system(command) != 0)
                        pdie("Cpio error");
                printf(".done\n");
        
                printf("Cleaning work directory. Removing files...");
                unlink(datafilename);
                unlink(keyfilename);
                unlink(encfilename);
                printf(".done\n"); 
        }
    
        printf("All operations successfully completed!\n");
    
        free(datafilename);
        free(keyfilename);
        free(encfilename);
        free(dataskeyfilename);
        free(keyskeyfilename);
        free(command);

        return 0;
    
}
