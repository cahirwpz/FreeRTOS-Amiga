TOPDIR = $(CURDIR)

SUBDIR = libc drivers FreeRTOS
SOURCES = startup.c trap.c main.c
BUILD-FILES = bootloader.bin freertos.exe freertos.adf

all: build

include $(TOPDIR)/build/build.gcc.mk

build-before: cscope tags 

FREERTOS = $(OBJECTS) drivers/drivers.lib FreeRTOS/freertos.lib libc/c.lib 

freertos.elf: $(FREERTOS)

freertos.adf: freertos.exe

a500rom.bin: a500rom.S build 
	@echo "[LD] $(addprefix $(DIR),$^) -> $(DIR)$@"
	$(AS) -Fbin $(ASFLAGS) -o $@ $(realpath $<)

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

run-floppy: build freertos.adf
	./launch -f freertos.adf -e freertos.elf

run-rom: build a500rom.bin freertos.adf
	./launch -r a500rom.bin -e freertos.elf -f freertos.adf

debug-floppy: build freertos.adf
	./launch -d -f freertos.adf -e freertos.elf

debug-rom: build a500rom.bin freertos.adf
	./launch -d -r a500rom.bin -e freertos.elf -f freertos.adf

clean-here:
	$(RM) *.adf *.bin *.elf *.exe *.map *.rom
	$(RM) *.o *~
	$(RM) cscope.out etags tags

.PHONY: debug-floppy debug-rom run-floppy run-rom cscope tags
