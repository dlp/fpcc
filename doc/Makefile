
TOOLBOX=fpcc

#TARGETS=$(addsuffix .1.gz, sig idx comp map diff help)
TARGETS=$(addsuffix .1.gz, sig)

.PHONY: all clean

all: $(TARGETS)

%.1.gz: %
	txt2man -t "$(TOOLBOX)-$*" -s1 $< | gzip > $@

clean:
	rm -f $(TARGETS)
