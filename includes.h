/*
 * Includes. Standart libraries.
 *
 * Copyright (C) 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <time.h>

#define _XOPEN_SOURCE 500
#include <unistd.h>
extern ssize_t pread(int, void *, size_t, off_t);

#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <linux/fs.h>



#include <errno.h>

