PREFIX = /opt/FreeRTOS-Amiga
BINDIR = $(PREFIX)/bin
TARGET = m68k-elf

export BINDIR

# Toolchain tools
CC = $(BINDIR)/$(TARGET)-gcc -ggdb
AS = $(BINDIR)/$(TARGET)-as
LD = $(BINDIR)/$(TARGET)-ld
AR = $(BINDIR)/$(TARGET)-ar
VASM = $(BINDIR)/vasm
RANLIB = $(BINDIR)/$(TARGET)-ranlib
READELF = $(BINDIR)/$(TARGET)-readelf
OBJCOPY = $(BINDIR)/$(TARGET)-objcopy
OBJDUMP = $(BINDIR)/$(TARGET)-objdump
ELF2HUNK = $(BINDIR)/elf2hunk

# Common tools used by actions
RM = rm -v -f
CSCOPE = cscope -b
ifneq ($(shell which uctags),)
CTAGS = uctags # under FreeBSD install universal-ctags
else
CTAGS = ctags
endif
FORMAT = clang-format -style=file
FSUTIL = $(TOPDIR)/tools/fsutil.py
LAUNCH = $(TOPDIR)/tools/launch
PNG2C = $(TOPDIR)/tools/png2c.py
PSF2C = $(TOPDIR)/tools/psf2c.py
GENSTRUCT = $(TOPDIR)/tools/genstruct.py
