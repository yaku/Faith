/*
 * Functions for making file names.
 *
 * Copyright (C) 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

void make_filename(int type, char* inputname, char *outputname) 
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


void make_out_time_filename(int type, char *outputname) 
{

        time_t localunixtime = time(NULL);
    
        struct tm *timestruct = localtime(&localunixtime);
        
        if (type == DATA)
            strftime(outputname, FILENAMEMAXLENGTH, "Data.out_%a.%d.%m.%Y_%H.%M.%S", timestruct);
            
        else if (type == KEY)
            strftime(outputname, FILENAMEMAXLENGTH, "Key.out_%a.%d.%m.%Y_%H.%M.%S", timestruct);

}

void make_time_filename(char *outputname) 
{

        time_t localunixtime = time(NULL);
    
        struct tm *timestruct = localtime(&localunixtime);

        strftime(outputname, FILENAMEMAXLENGTH, "Data_%a_%d.%m.%Y_%H.%M.%S.cpio", timestruct);
        
}
