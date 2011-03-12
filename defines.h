/*
 * Constants.
 *
 * Copyright (C) 2010, 2011 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

#define ENCRYPT 0
#define DECRYPT 1

#define DATA 0
#define KEY 1

#define FILENAMEMAXLENGTH 512
#define OUTDATAFILE 0
#define KEYFILE 1
#define ENCFILE 2
#define DATASKEYFILE 3
#define KEYSKEYFILE 4

#define BLOCKSIZE 512
#define BUFFER_SIZE 16777216

#define BUFFERED_BLOCKS (BUFFER_SIZE / BLOCKSIZE)

#define FILENAMEMAXLENGTH 512

#define SKEYRECORDSIZE 5

#define MAXLEVEL 2000

#define CONFIGSTRINGMAXLEN 1024

#define DEVURANDOM 1
#define SLRAND 2

#define STEP 4096

#define FORWARD  1
#define BACKWARD 2
#define BAD      3

