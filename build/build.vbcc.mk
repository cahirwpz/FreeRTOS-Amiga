PREFIX = $(TOPDIR)/toolchain

CC = $(PREFIX)/bin/vc
AS = $(PREFIX)/bin/vasm
LD = $(PREFIX)/bin/vlink

ASFLAGS = -m68010 -quiet
CFLAGS = -c99 -cpu=68000 -O3 -size # -warn=-1 -dontwarn=307
CPPFLAGS = -I$(TOPDIR)/FreeRTOS/include \
	   -I$(TOPDIR)/FreeRTOS/portable/VBCC/m68k \
	   -I$(TOPDIR)/toolchain/m68k-amigaos/include \
	   -I$(TOPDIR)
LDFLAGS = -S

include $(TOPDIR)/build/compile.mk
include $(TOPDIR)/build/common.mk
