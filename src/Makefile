
SHELL := /bin/bash

CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -g -D_DEFAULT_SOURCE
LDFLAGS =

ifeq ($(DEBUG),1)
  CFLAGS += -DDEBUG
endif

C_TOOLS = sig comp idx map

COMMON_OBJ = common.o

.PHONY: all clean


all: $(C_TOOLS)

$(addsuffix .o, $(C_TOOLS)) common.o: common.h

%.o: %.c
	$(CC) $(CFLAGS) -c $<

# rule to build a tool
%: %.o $(COMMON_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

# rules related to sig
lex.yy.o: lex.yy.c ccode.tab.h

ccode.tab.c ccode.tab.h : ccode.y
	bison -d $<

lex.yy.c: ccode.lex
	flex $<

sig: LDLIBS = -lcrypto
sig: lex.yy.o


clean:
	rm -f ccode.tab.{c,h} lex.yy.c *.o
	rm -f $(C_TOOLS)
