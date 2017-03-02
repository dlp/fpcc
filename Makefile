
SHELL := /bin/bash

TOOL_PREFIX=fpcc-

# fpcc tools created from C source code
C_TOOLS = $(addprefix src/, sig comp idx map)

# fpcc tools which are scripts
S_TOOLS = $(addprefix utils/, fpcc-help fpcc-diff fpcc)

# collect all into FPCC_TOOLS
FPCC_TOOLS  = $(C_TOOLS:src/%=bin/$(TOOL_PREFIX)%)
FPCC_TOOLS += $(S_TOOLS:utils/%=bin/%)

SUBDIRS = src doc

.PHONY: all clean $(SUBDIRS)

all: $(SUBDIRS) $(FPCC_TOOLS)

$(FPCC_TOOLS): $(SUBDIRS)

bin/$(TOOL_PREFIX)%: src/%
	install -D -T $< $@

bin/%: utils/%
	install -D $< $@

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	rm -fr bin
	$(MAKE) -C src clean
	$(MAKE) -C doc clean
