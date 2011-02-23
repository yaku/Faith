CFLAGS = -mtune=i686 -D_FILE_OFFSET_BITS=64 -O2

CC = gcc
LIBS =
INCLUDES =
OBJS = faith.o
SRCS = faith.c
HDRS =  


all: faith clean

faith: faith.o
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${OBJS} ${LIBS}

clean:
	rm *.o 

