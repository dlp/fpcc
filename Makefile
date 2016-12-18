
SHELL := /bin/bash

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g
LDFLAGS =

OBJS =

.PHONY: all clean

all: csig

%.o: %.c
	$(CC) $(CFLAGS) -c $<

lex.yy.o: lex.yy.c ccode.tab.h

ccode.tab.c ccode.tab.h : ccode.y
	bison -d $<

lex.yy.c: ccode.lex
	flex $<


csig: LDLIBS = -lcrypto
csig: csig.o lex.yy.o

clean:
	rm -f ccode.tab.{c,h} lex.yy.c *.o ccode.output
	rm -f csig
