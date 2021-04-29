TOPDIR = $(realpath ../../../..)

PROGRAMS = zepto
SOURCES = raw.c command.c display.c gap.c key.c search.c main.c

CPPFLAGS = -I$(TOPDIR)/libc/include -D_USERSPACE

all: build

include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/common.mk

%: %.elf
	@echo "[ELF2HUNK] $(DIR)$< -> $(DIR)$@"
	$(ELF2HUNK) $< $@

%.elf: crt0.o %.o $(TOPDIR)/libc/c.lib
	@echo "[LD] $(addprefix $(DIR), $*.o) -> $(DIR)$@"
	$(LD) $(LDFLAGS) -o $@ $^
