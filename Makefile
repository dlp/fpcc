
SHELL := /bin/bash

CC := gcc
CFLAGS := -std=c99 -pedantic -Wall -D_XOPEN_SOURCE=500 -D_BSD_SOURCE -g
LDFLAGS :=

OBJS =

.PHONY: all clean

all: fpcc

fpcc: $(OBJS) lex.yy.o
	$(CC) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $<

ccode.tab.c ccode.tab.h : ccode.y
	bison -d $<

lex.yy.c: ccode.lex ccode.tab.h
	flex $<

clean:
	rm -f ccode.tab.{c,h} lex.yy.c *.o ccode.output
	rm -f fpcc
