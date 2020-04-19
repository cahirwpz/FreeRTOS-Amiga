TOPDIR = $(realpath ../..)

PROGRAM = loadexec
SOURCES = main.c memory.c
OBJECTS = ../startup.o ../fault.o ../trap.o test.exe.o

CLEAN-FILES = test.o test.bin.o test.exe test.elf test.elf.map

include $(TOPDIR)/build/build.prog.mk

test.elf: test.o
	@echo "[LD] $(DIR)$< -> $(DIR)$@"
	$(LD) $(LDFLAGS) -Map $@.map -o $@ $<
