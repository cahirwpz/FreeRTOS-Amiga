TOPDIR = $(realpath ../..)

PROGRAM = loadexec
SOURCES = main.c usermode.c trampoline.S
OBJECTS = ../startup.o ../fault.o ../trap.o test.exe.o

CLEAN-FILES = test.o syscalls.o test.bin.o test.exe test.elf test.elf.map

include $(TOPDIR)/build/build.prog.mk

test.elf: test.o syscall.o
	@echo "[LD] $(addprefix $(DIR), $^) -> $(DIR)$@"
	$(LD) $(LDFLAGS) -Map $@.map -o $@ $^
