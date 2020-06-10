TOPDIR = $(realpath ../..)

PROGRAM = loadexec
SOURCES = main.c sysent.c proc.c trampoline.S
OBJECTS = ../startup.o ../fault.o ../trap.o ushell.exe.o ucat.exe.o

CLEAN-FILES = crt0.o sysapi.o shell.o cat.o
CLEAN-FILES += ushell.bin.o ushell.exe ushell.elf ushell.elf.map
CLEAN-FILES += ucat.bin.o ucat.exe ucat.elf ucat.elf.map

include $(TOPDIR)/build/build.prog.mk

u%.elf: crt0.o %.o sysapi.o $(TOPDIR)/libc/c.lib
	@echo "[LD] $(addprefix $(DIR), $^) -> $(DIR)$@"
	$(LD) $(LDFLAGS) -Map $@.map -o $@ $^
