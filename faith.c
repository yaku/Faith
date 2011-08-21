/*
 * Faith Crypto. Immersion theory cipher.
 * Copyright (C) 2010, 2011 Denis Yakunchikov <toi.yaku@gmail.com>
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
#include "filename.h"
#include "config.h"
#include "password.h"
#include "immer.h"
#include "gammacipher.h"

void print_usage()
{
        printf("\
Usage: faith -[e,d] [-l filelist] [-p pass] [-f size] device\n\
Examples:\n\
Encrypt files in filelist to device named /dev/sdb1\n\
faith -e -l filelist /dev/sdb1\n\
Encrypt files in filelist to file named testfile with size 100 megabytes\n\
faith -e -l filelist -f 100M testfile\n\n\
Decrypt device or file with password pass\n\
faith -d -p pass /dev/sdb1\n\
faith -d -p pass testfile\n");
        exit(0);
}

void remove_files(int mode, names filename) {

        unlink(filename.key);
        unlink(filename.enc);
        unlink(filename.data);

        if (mode == ENCRYPT) {
                unlink(filename.dataskey);
                unlink(filename.keyskey);
		unlink("space.1");
		unlink("space.2");
        }

}

off_t arg_file_size(char *argument) {

        off_t result = 0;

        while (*argument) {
                switch (*argument) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                        result *= 10;
                        result += (*argument - '0');

                        break;
                case 'k':
                case 'K':
                        if (result == 0)
                                result = 1;
                        result *= 1024;
                        break;
                case 'm':
                case 'M':
                        if (result == 0)
                                result = 1;
                        result *= (1024 * 1024);
                        break;
                case 'g':
                case 'G':
                        if (result == 0)
                                result = 1;
                        result *= (1024 * 1024 * 1024);
                        break;
                }

                argument++;
        }

        return result;

}

int main(int argc, char **argv)
{

        int mode = 0;

        /*Check comand line arguments*/
        if (argc == 1)
                print_usage();

        if (strcmp(argv[1], "-d") == 0)
                mode = DECRYPT;

        else if (strcmp(argv[1], "-e") == 0)
                mode = ENCRYPT;

        else
                print_usage();

        config conf = {0, 0};

        int i;
        char *list = NULL;
        uchar *pass = NULL;
        char *name = NULL;
        
        int needargs = 1;

        for (i = 2; i < argc; i += 2) {

                if (argv[i][0] == '-')
                        switch (argv[i][1]) {
                        case 'l':
                                if (mode == DECRYPT)
                                        print_usage();

                                else {
                                        list = argv[i + 1];
                                        needargs++;
                                }

                                break;

                        case 'p':
                                if (mode == ENCRYPT)
                                        print_usage();

                                else {
                                        pass = (uchar *) argv[i + 1];
                                        needargs++;
                                }

                                break;

                        case 'f':
                                if (mode == DECRYPT)
                                        print_usage();

                                else {
                                        conf.isfile = 1;
                                        conf.filesize = arg_file_size(argv[i + 1]);

                                        if (conf.filesize == 0)
                                                print_usage();
                                }

                                break;
                        }
                else {
                        name = argv[i];
                        i -= 1;
                        needargs++;
                }


        }

        if (needargs != 3)
                print_usage();
        /*End of check comand line arguments*/

        names filename  = {
                malloc(FILENAMEMAXLENGTH),
                malloc(FILENAMEMAXLENGTH),
                malloc(FILENAMEMAXLENGTH),
                malloc(FILENAMEMAXLENGTH),
                malloc(FILENAMEMAXLENGTH)
        };

        char *command = malloc(1024);

        printf("Initialising random number numbers generator.\n");
	uint seed = 0;
	int randdev = open("/dev/random", O_RDONLY);
	if (read(randdev, &seed, sizeof(uint)) < 0)
		pdie("Read failed");
        srand(seed);
	close(randdev);

        printf("Making names.\n");
        make_names(mode, filename);

        if (mode == ENCRYPT) {

                printf("Encryption started\n");

                printf("Running cpio. Making archive.\n");
                snprintf(command, 1024, "cpio -o -O %s < %s", filename.data, list);
                if (system(command) != 0)
                        pdie("Cpio error");

                printf("Gamma cipher encryption\n");
                gammacipher_main(mode, filename);
                
                printf("Diving data into device\n");
                immer_main(mode, name, filename, 0, conf);
                
        }

        if (mode == DECRYPT) {

                printf("Decryption started\n");

                printf("Re-diving data from device\n");
                immer_main(mode, name, filename, pass, conf);

                printf("Gamma cipher decryption\n");
                gammacipher_main(mode, filename);

                printf("Running cpio. Extractiong archive.\n");
                snprintf(command, 1024, "cpio -id -I %s", filename.enc);
                if (system(command) != 0)
                        pdie("Cpio error");

        }

        printf("Cleaning work directory. Removing files.\n");
	remove_files(mode, filename);

        printf("All operations successfully completed!\n");

        free(filename.data);
        free(filename.key);
        free(filename.enc);
        free(filename.dataskey);
        free(filename.keyskey);
        free(command);

        return 0;
}
