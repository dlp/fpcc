
SHELL := /bin/bash

TOOL_PREFIX=fpcc-

# fpcc tools created from C source code
C_TOOLS = $(addprefix src/, sig comp idx map)

# fpcc tools which are scripts
S_TOOLS = $(wildcard utils/*)

# collect all into FPCC_TOOLS
FPCC_TOOLS  = $(C_TOOLS:src/%=bin/$(TOOL_PREFIX)%)
FPCC_TOOLS += $(S_TOOLS:utils/%=bin/%)

.PHONY: all clean src doc

all: src doc
	$(MAKE) $(FPCC_TOOLS)


bin/$(TOOL_PREFIX)%: src/%
	install -D -T $< $@


bin/%: utils/%
	install -D $< $@

src doc:
	$(MAKE) -C $@

clean:
	rm -fr bin
	$(MAKE) -C src clean
	$(MAKE) -C doc clean
