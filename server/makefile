
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror 
LDFLAGS = -lrt -pthread 

all: aesdsocket

aesdsocket: aesdsocket.o
	 $(CC) $(CFLAGS) -o aesdsocket aesdsocket.o $(LDFLAGS)
aesdsocket.o: aesdsocket.c
	 $(CC) $(CFLAGS) -c aesdsocket.c queue.h

clean: 
	 rm -f  *.o  \
	        aesdsocket
