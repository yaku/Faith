/*
 * Config structure and function for reading the configuration file.
 *
 * Copyright (C) 2010 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */
 
 
typedef struct config {

    char randomizer;

} config;

void read_config(config *confstruct, char *configfilename) {

    char *configstring=malloc(CONFIGSTRINGMAXLEN);
    
    FILE *configfile = fopen(configfilename,"r");
    
    if (configfile==NULL)
        pdie("Open configuration file failed");
    
    confstruct->randomizer=0;
    
    while (feof(configfile)==0) {
        
        fgets(configstring,CONFIGSTRINGMAXLEN,configfile);
    
        if (strcmp(configstring,"randomizer=URANDOM\n")==0)
            confstruct->randomizer=URANDOM;
        else if (strcmp(configstring,"randomizer=SRAND\n")==0)
            confstruct->randomizer=SRAND;
    
    }
    
    if (confstruct->randomizer==0) {
    
        printf("Geting randomizer from configuration file failed.\nSetting default URANDOM\n");
        confstruct->randomizer=URANDOM;
    
    }

}
