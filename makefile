CFLAGS = -pg

CC = gcc
LIBS =  -lm 
INCLUDES =
OBJS = faith.o
SRCS = faith.c
HDRS =  


all: faith clean

# The variable $@ has the value of the target. In this case $@ = psort
faith: faith.o
	${CC} ${CFLAGS} ${INCLUDES} -o $@ ${OBJS} ${LIBS}

clean:
	rm *.o 

