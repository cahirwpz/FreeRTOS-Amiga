PREFIX = /opt/FreeRTOS-Amiga
BINDIR = $(PREFIX)/bin

CC = $(BINDIR)/m68k-elf-gcc -ggdb
AS = $(BINDIR)/vasm
LD = $(BINDIR)/m68k-elf-ld
AR = $(BINDIR)/m68k-elf-ar
RANLIB = $(BINDIR)/m68k-elf-ranlib
READELF = $(BINDIR)/m68k-elf-readelf
OBJCOPY = $(BINDIR)/m68k-elf-objcopy
OBJDUMP = $(BINDIR)/m68k-elf-objdump
ELF2HUNK = $(BINDIR)/elf2hunk

ASFLAGS = -m68010 -quiet -I$(PREFIX)/m68k-amigaos/include
CFLAGS = -std=gnu11 $(OFLAGS) $(WFLAGS)
OFLAGS = -m68010 -Wall -fomit-frame-pointer -Os -fno-common # -mpcrel
WFLAGS = -Wall -Wno-builtin-declaration-mismatch
CPPFLAGS = -I$(TOPDIR)/FreeRTOS/portable/m68k-amiga \
	   -I$(TOPDIR)/include \
	   -I$(TOPDIR)
LDFLAGS = -nostdlib

%.o: %.S
	@echo "[AS] $(DIR)$< -> $(DIR)$@"
	$(AS) -Felf $(CPPFLAGS) $(ASFLAGS) -o $@ $(realpath $<)

%.lib:
	@echo "[AR] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AR) rcs $@ $^
	$(RANLIB) $@

%.elf:
	@echo "[LD] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(LD) $(LDFLAGS) --emit-relocs -T $(TOPDIR)/amiga.ld -Map $@.map -o $@ $^

%.exe:	%.elf
	@echo "[ELF2HUNK] $(DIR)$< -> $(DIR)$@"
	$(ELF2HUNK) $< $@

include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk
