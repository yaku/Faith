/*
 * pdie function.
 *
 * Copyright (C) 2010 Denis Yakunchikov <toi.yaku@gmail.com>
 *
 * This file is licensed under GPLv3.
 *
 */

void pdie(const char *mesg) {
   perror(mesg);
   exit(1);
}
