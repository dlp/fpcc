###############################################################################
#
# Makefile for fpcc
#
# Daniel Prokesch <daniel.prokesch@gmail.com>
###############################################################################

SUITE = fpcc
TOOL_PREFIX = $(SUITE)-

# all the tools in the resulting bin directory
SUITE_TOOLS = $(addprefix bin/, $(SUITE) \
		$(addprefix $(TOOL_PREFIX), sig comp idx map help diff))

# the manpages are generated from the doc/*.txt files
MANTXT = $(wildcard doc/*.txt)
MANPAGES = $(MANTXT:.txt=.1.gz)


###############################################################################
CC = gcc
CFLAGS = -std=c99 -pedantic -Wall -g -D_DEFAULT_SOURCE
LDFLAGS =

ifeq ($(DEBUG),1)
  CFLAGS += -DDEBUG
endif
ifeq ($(RELEASE),1)
  CFLAGS = -std=c99 -pedantic -Wall -O3 -D_DEFAULT_SOURCE -DNDEBUG
endif
###############################################################################


.PHONY: all clean tools docs list

all: tools docs

tools: $(SUITE_TOOLS)
docs: $(MANPAGES)

$(SUITE_TOOLS): | bin

bin:
	test -e $@ -a -d $@ || mkdir $@

# shared header
$(patsubst %.c, %.o, $(wildcard src/*.c)): src/common.h

COMMON_OBJ = src/common.o

# rule to build a C tool
bin/$(TOOL_PREFIX)%: src/%.o $(COMMON_OBJ)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

# rules related to sig
src/lex.yy.o: src/lex.yy.c src/ccode.tab.h

# we are only interested in the header for the tokens
src/ccode.tab.h: src/ccode.y
	bison --defines=$@ --output=/dev/null $<

src/lex.yy.c: src/ccode.lex
	flex -o $@ $<

bin/$(TOOL_PREFIX)sig: LDLIBS = -lcrypto
bin/$(TOOL_PREFIX)sig: src/lex.yy.o


bin/%: utils/%
	cp $< $@

%.1.gz: %.txt
	txt2man -t "$(TOOL_PREFIX)$(*F)" -s1 $< | gzip > $@

# Generate a list of tools with their description.
# This assumes that this information is in the second line of the doc/.txt file.
list:
	@for t in $(MANTXT); do \
	  head -n2 $$t | tail -n1 | \
	  sed -e "s/$(TOOL_PREFIX)\([[:alnum:]]\+\)[[:space:]]\+-[[:space:]]\+\(.*\)$$/\1\t\t\2/" ;\
	  done

clean:
	rm -fr bin
	rm -f src/ccode.tab.[ch] src/lex.yy.c src/*.o
	rm -f $(MANPAGES)




