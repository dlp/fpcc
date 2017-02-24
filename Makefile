
SHELL := /bin/bash

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -g -D_DEFAULT_SOURCE
LDFLAGS =

ifeq ($(DEBUG),1)
  CFLAGS += -DDEBUG
endif


.PHONY: all clean

all: fpcc-sig fpcc-comp fpcc-idx fpcc-map

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

# rules related to fpcc-idx
fpcc-idx.o: common.h
fpcc-idx: fpcc-idx.o common.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# rules related to fpcc-map
fpcc-map.o: common.h
fpcc-map: fpcc-map.o common.o
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

clean:
	rm -f ccode.tab.{c,h} lex.yy.c *.o
	rm -f fpcc-sig fpcc-comp fpcc-idx fpcc-map
