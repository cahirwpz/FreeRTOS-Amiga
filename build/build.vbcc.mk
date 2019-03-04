PREFIX = $(TOPDIR)/toolchain

CC = $(PREFIX)/bin/vc
AS = $(PREFIX)/bin/vasm
LD = $(PREFIX)/bin/vlink

ASFLAGS = -quiet
CFLAGS = -c99 -cpu=68000 -O=2 # -warn=-1 -dontwarn=307
CPPFLAGS = -I$(TOPDIR)/FreeRTOS/include \
	   -I$(TOPDIR)/FreeRTOS/portable/VBCC/m68k \
	   -I$(TOPDIR)/toolchain/m68k-amigaos/include \
	   -I$(TOPDIR)
LDFLAGS = -S -s

include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk
