
SHELL := /bin/bash

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g
LDFLAGS =

.PHONY: all clean

all: fpcc-sig fpcc-comp

%.o: %.c
	$(CC) $(CFLAGS) -c $<

common.o: common.h

# rules related to fpcc-sig
lex.yy.o: lex.yy.c ccode.tab.h

ccode.tab.c ccode.tab.h : ccode.y
	bison -d $<

lex.yy.c: ccode.lex
	flex $<

fpcc-sig.o: common.h
fpcc-sig: LDLIBS = -lcrypto
fpcc-sig: fpcc-sig.o lex.yy.o common.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# rules related to fpcc-comp
fpcc-comp.o: common.h
fpcc-comp: fpcc-comp.o common.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f ccode.tab.{c,h} lex.yy.c *.o
	rm -f fpcc-sig fpcc-comp
