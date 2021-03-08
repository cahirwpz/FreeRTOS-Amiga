TOPDIR = $(realpath ../..)

PROGRAMS = cat echo grep init kill ls mkdir rm sh wc
SOURCES = $(addsuffix .c,$(PROGRAMS))

BUILD-FILES += $(PROGRAMS)
CLEAN-FILES += $(addsuffix .elf,$(PROGRAMS))

CPPFLAGS = -I$(TOPDIR)/xv6/include

all: build

include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/common.mk

%: %.elf
	@echo "[ELF2HUNK] $(DIR)$< -> $(DIR)$@"
	$(ELF2HUNK) $< $@

%.elf: ../libc/crt0.o %.o ../libc/c.lib
	@echo "[LD] $(addprefix $(DIR), $^) -> $(DIR)$@"
	$(LD) $(LDFLAGS) -o $@ $^

