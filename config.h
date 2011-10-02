/*
 * Config structure and function for reading the configuration file.
 *
 * Copyright (C) 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

typedef struct config {

        char isfile;
        off_t filesize;
        char fillrandom;

} config;
