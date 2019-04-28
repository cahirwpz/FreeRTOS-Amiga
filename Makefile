TOPDIR = $(CURDIR)

SUBDIR = libc drivers FreeRTOS
SOURCES = startup.c trap.c main.c

all: build

include $(TOPDIR)/build/build.gcc.mk

build-before: cscope tags 
build-here: bootloader.bin freertos.adf

freertos.elf: $(OBJECTS) drivers/drivers.lib FreeRTOS/freertos.lib libc/c.lib 

freertos.adf: freertos.exe

# Lists of all files that we consider our sources.
SRCDIRS = include drivers libc FreeRTOS 
SRCFILES_C = $(shell find $(SRCDIRS) -iname '*.[ch]')
SRCFILES_ASM = $(shell find $(SRCDIRS) -iname '*.S')
SRCFILES = $(SRCFILES_C) $(SRCFILES_ASM)

cscope: $(SRCFILES)
	@echo "[CSCOPE] Rebuilding database..."
	$(CSCOPE) $(SRCFILES)

tags:
	@echo "[CTAGS] Rebuilding tags..."
	$(CTAGS) --language-force=c $(SRCFILES_C)
	$(CTAGS) --language-force=c -e -f etags $(SRCFILES_C)
	$(CTAGS) --language-force=asm -a $(SRCFILES_ASM)
	$(CTAGS) --language-force=asm -aef etags $(SRCFILES_ASM)

toolchain:
	make -C external/vbcc PREFIX=$(PWD)/toolchain
	make -C external/gnu PREFIX=$(PWD)/toolchain
	make -C external/elf2hunk PREFIX=$(PWD)/toolchain
	make -C external/fs-uae PREFIX=$(PWD)/toolchain

run: freertos.adf
	./launch $^

clean-here:
	$(RM) bootloader.bin
	$(RM) *.adf *.exe *~
	$(RM) cscope.out etags tags

.PHONY: toolchain run cscope tags
