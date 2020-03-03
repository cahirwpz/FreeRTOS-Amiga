PREFIX = /opt/FreeRTOS-Amiga
BINDIR = $(PREFIX)/bin
TARGET = m68k-elf

export BINDIR

# Toolchain tools
CC = $(BINDIR)/$(TARGET)-gcc -ggdb
AS = $(BINDIR)/vasm
LD = $(BINDIR)/$(TARGET)-ld
AR = $(BINDIR)/$(TARGET)-ar
RANLIB = $(BINDIR)/$(TARGET)-ranlib
READELF = $(BINDIR)/$(TARGET)-readelf
OBJCOPY = $(BINDIR)/$(TARGET)-objcopy
OBJDUMP = $(BINDIR)/$(TARGET)-objdump
ELF2HUNK = $(BINDIR)/elf2hunk

# Common tools used by actions
RM = rm -v -f
CSCOPE = cscope -b
CTAGS = ctags
FORMAT = clang-format -style=file
FSUTIL = $(TOPDIR)/tools/fsutil.py
LAUNCH = $(TOPDIR)/tools/launch
