TOPDIR = $(realpath .)

SUBDIR = tools FreeRTOS drivers libc examples
BUILD-FILES = bootloader.bin cscope.out tags etags

all: build

include $(TOPDIR)/build/flags.mk
include $(TOPDIR)/build/common.mk

%.bin: %.asm
	@echo "[AS] $(DIR)$< -> $(DIR)$@"
	$(VASM) -Fbin $(CPPFLAGS) $(VASMFLAGS) -o $@ $(realpath $<)

before-examples: bootloader.bin build-FreeRTOS build-drivers build-libc
before-FreeRTOS: build-tools
before-drivers: build-tools
before-libc: build-tools

# Lists of all files that we consider our sources.
SRCDIRS = include drivers libc FreeRTOS 
SRCFILES_C = $(shell find $(SRCDIRS) -iname '*.[ch]')
SRCFILES_ASM = $(shell find $(SRCDIRS) -iname '*.S')
SRCFILES = $(SRCFILES_C) $(SRCFILES_ASM)

cscope.out: $(SRCFILES)
	@echo "[CSCOPE] Rebuilding database..."
	$(CSCOPE) $(SRCFILES)

tags etags: $(SRCFILES)
	@echo "[CTAGS] Rebuilding tags..."
	$(CTAGS) --language-force=c $(SRCFILES_C)
	$(CTAGS) --language-force=c -e -f etags $(SRCFILES_C)
	$(CTAGS) --language-force=asm -a $(SRCFILES_ASM)
	$(CTAGS) --language-force=asm -aef etags $(SRCFILES_ASM)

# vim: ts=8 sw=8 noet
