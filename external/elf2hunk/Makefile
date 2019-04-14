CURL = curl -L
RM = rm -f
INSTALL = install

CC = gcc -g
CFLAGS = -O2 -Wall

PREFIX ?= $(PWD)/local

all: elf2hunk-install

elf2hunk.c:
	$(CURL) -o $@ "https://repo.or.cz/AROS.git/blob_plain/HEAD:/tools/elf2hunk/elf2hunk.c"

elf2hunk-patch: .pc/applied-patches
.pc/applied-patches:
	quilt push -a

elf2hunk-unpatch: .pc/applied-patches
	quilt pop -a

elf2hunk-build: elf2hunk-patch elf2hunk

elf2hunk-install: elf2hunk-build
	$(INSTALL) -m 755 elf2hunk $(PREFIX)/bin

clean: elf2hunk-unpatch
	$(RM) elf2hunk *~
	$(RM) -r elf2hunk.dSYM

.PHONY: elf2hunk-build elf2hunk-patch elf2hunk-unpatch elf2hunk-install
