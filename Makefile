
SHELL := /bin/bash

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g
LDFLAGS =

OBJS =

.PHONY: all clean

all: csig comp

%.o: %.c
	$(CC) $(CFLAGS) -c $<


# rules related to csig

lex.yy.o: lex.yy.c ccode.tab.h

ccode.tab.c ccode.tab.h : ccode.y
	bison -d $<

lex.yy.c: ccode.lex
	flex $<

csig.o: common.h

csig: LDLIBS = -lcrypto
csig: csig.o lex.yy.o


# rules related to comp

comp.o: common.h

comp: comp.o

clean:
	rm -f ccode.tab.{c,h} lex.yy.c *.o
	rm -f csig comp
