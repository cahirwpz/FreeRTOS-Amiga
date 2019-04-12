CURL = curl -L
RM = rm -f
INSTALL = install

CC = gcc
CFLAGS = -O2 -Wall

PREFIX ?= $(PWD)/local

all: elf2hunk-install
	
elf2hunk.c:
	$(CURL) -o $@ "https://repo.or.cz/AROS.git/blob_plain/HEAD:/tools/elf2hunk/elf2hunk.c"

elf2hunk-install: elf2hunk
	$(INSTALL) -m 755 $^ $(PREFIX)/bin

clean:
	$(RM) -f elf2hunk

.PHONY: elf2hunk-install
