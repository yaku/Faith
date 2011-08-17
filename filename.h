/*
 * Functions for making file names.
 *
 * Copyright (C) 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

typedef struct names {

        char *data;
        char *key;
        char *dataskey;
        char *keyskey;
        char *enc;

} names;

void make_filename(int type, char *inputname, char *outputname)
{

        switch (type) {
        case OUTDATAFILE:
                snprintf(outputname, FILENAMEMAXLENGTH, "%s.out", inputname);
                break;
        case KEYFILE:
                snprintf(outputname, FILENAMEMAXLENGTH, "%s.key", inputname);
                break;
        case ENCFILE:
                snprintf(outputname, FILENAMEMAXLENGTH, "%s.enc", inputname);
                break;
        case DATASKEYFILE:
                snprintf(outputname, FILENAMEMAXLENGTH, "%s.dataskey", inputname);
                break;
        case KEYSKEYFILE:
                snprintf(outputname, FILENAMEMAXLENGTH, "%s.keyskey", inputname);
                break;
        default:
                pdie("Internal error");
        }
}


void make_out_time_filename(int type, char *outname)
{

        time_t localunixtime = time(NULL);

        struct tm *timestruct = localtime(&localunixtime);

        if (type == DATA)
            strftime(outname, FILENAMEMAXLENGTH, "Data.out_%a.%d.%m.%Y_%H.%M.%S",
            							    timestruct);

        else if (type == KEY)
            strftime(outname, FILENAMEMAXLENGTH, "Key.out_%a.%d.%m.%Y_%H.%M.%S",
            							    timestruct);

}

void make_time_filename(char *outname)
{

        time_t localunixtime = time(NULL);

        struct tm *timestruct = localtime(&localunixtime);

        strftime(outname, FILENAMEMAXLENGTH, "Data_%a_%d.%m.%Y_%H.%M.%S.cpio", 
        							    timestruct);

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

