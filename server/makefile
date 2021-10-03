CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -Werror

all: server

server: aesdsocket.o
	 $(CC) $(CFLAGS) -o server aesdsocket.o
aesdsocket.o: aesdsocket.c
	 $(CC) $(CFLAGS) -c aesdsocket.c

clean: 
	 rm -f  *.o  \
	        server


