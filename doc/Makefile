
TOOLBOX=fpcc

TOOLS=sig idx comp map diff help
TOOLS_TXT=$(addsuffix .txt, $(TOOLS))

TARGETS=$(TOOLS_TXT:.txt=.1.gz)

.PHONY: all clean list

all: $(TARGETS)

%.1.gz: %.txt
	txt2man -t "$(TOOLBOX)-$*" -s1 $< | gzip > $@

# Generate a list of tools with their description.
# This assumes that this information is in the second line of the .txt file.
list:
	@for t in $(TOOLS_TXT); do \
	  head -n2 $$t | tail -n1 ;\
	  done

clean:
	rm -f $(TARGETS)
