TOPDIR = $(CURDIR)

SUBDIR = libc drivers FreeRTOS
SOURCES = startup.c trap.c main.c

all: build

include $(TOPDIR)/build/build.vbcc.mk

build-before: cscope tags 
build-here: bootloader.bin freertos.adf

freertos.exe: $(OBJECTS) drivers/drivers.lib libc/c.lib FreeRTOS/freertos.lib

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

vbcc:
	make -C external/vbcc DESTDIR=$(PWD)/toolchain

run: freertos.adf
	./launch $^

clean-here:
	$(RM) bootloader.bin
	$(RM) *.adf *.exe *~
	$(RM) cscope.out etags tags

.PHONY: vbcc run cscope tags
