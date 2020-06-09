TOPDIR = $(realpath ../..)

PROGRAM = loadexec
SOURCES = main.c sysent.c proc.c trampoline.S
OBJECTS = ../startup.o ../fault.o ../trap.o shell.exe.o

CLEAN-FILES = crt0.o sysapi.o 
CLEAN-FILES += shell.o shell.bin.o shell.exe shell.elf shell.elf.map

include $(TOPDIR)/build/build.prog.mk

shell.elf: crt0.o shell.o sysapi.o $(TOPDIR)/libc/c.lib
	@echo "[LD] $(addprefix $(DIR), $^) -> $(DIR)$@"
	$(LD) $(LDFLAGS) -Map $@.map -o $@ $^
