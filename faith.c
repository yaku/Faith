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
#include "config.h"
#include "password.h"
#include "immer.h"
#include "gammacipher.h"
#include "filename.h"

void print_usage()
{
        printf("Usage: faith -[e,d] device [-l filelist] [-p pass] [-f size]\n");
        exit(0);
}

void make_names(int mode, names filename) {

        make_time_filename(filename.data);
        make_filename(KEYFILE, filename.data, filename.key);
        make_filename(ENCFILE, filename.data, filename.enc);
        make_filename(DATASKEYFILE, filename.data, filename.dataskey);
        make_filename(KEYSKEYFILE, filename.data, filename.keyskey);

        if (mode == DECRYPT) {
                make_out_time_filename(DATA, filename.data);
                make_out_time_filename(KEY, filename.key);
        }

}

void remove_files(int mode, names filename) {

        unlink(filename.key);
        unlink(filename.enc);
        unlink(filename.data);

        if (mode == ENCRYPT) {
                unlink(filename.dataskey);
                unlink(filename.keyskey);
        }

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

        int i;
        char *list, *pass, *name;

        for (i = 2; i < argc; i += 2) {

                if (argv[i][0] == '-')
                        switch (argv[i][1]) {
                        case 'l':
                                if (mode == DECRYPT)
                                        print_usage();
                                else
                                        list = argv[i + 1];

                                break;

                        case 'p':
                                if (mode == ENCRYPT)
                                        print_usage();
                                else
                                        pass = argv[i + 1];

                                break;
                        }
                else {
                        name = argv[i];
                        i -= 1;
                }


        }

        if (argc < 5)
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

        printf("Initialising random number numbers generator...");
        srand(time(NULL));
        printf(".done\n");

        printf("Reading configuration file.\n");
        config conf;
        read_config(&conf, "config.cfg");

        printf("Making names.\n");
        make_names(mode, filename);

        if (mode == ENCRYPT) {

                printf("Encryption started\n");

                printf("Running cpio. Making archive...");
                snprintf(command, 1024, "cpio -o -O %s < %s", filename.data, list);
                if (system(command) != 0)
                        pdie("Cpio error");
                printf(".done\n");

                printf("Main gamma cipher encryption started...");
                gammacipher_main(mode, filename, conf);
                printf(".done\n");

                printf("Dive data to device started...");
                immer_main(mode, name, filename, 0, conf);
                printf(".done\n");

        }

        if (mode == DECRYPT) {

                printf("Decryption started\n");

                printf("Re-diving data from device...");
                immer_main(mode, name, filename, pass, conf);
                printf(".done\n");

                printf("Main gamma cipher decryption started...");
                gammacipher_main(mode, filename, conf);
                printf(".done\n");

                printf("Running cpio. Extractiong archive...");
                snprintf(command, 1024, "cpio -id -I %s", filename.data);
                if (system(command) != 0)
                        pdie("Cpio error");
                printf(".done\n");

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

