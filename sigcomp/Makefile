CC = gcc
CFLAGS = -Wall -ansi -pedantic -O2
DEBUG = -d

.PHONY: all clean

all:	sig comp

sig:	sig.c
	$(CC) $(CFLAGS) -o sig sig.c

comp:	comp.c
	$(CC) $(CFLAGS) -o comp comp.c

clean:
	rm -f sig comp
