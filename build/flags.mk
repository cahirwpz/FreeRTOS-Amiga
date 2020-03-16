ASFLAGS   += -Wa,--register-prefix-optional -Wa,--bitwise-or
VASMFLAGS += -m68010 -quiet -I$(PREFIX)/m68k-amigaos/include
CFLAGS    += -std=gnu11 $(OFLAGS) $(WFLAGS)
OFLAGS    += -m68010 -Wall -fomit-frame-pointer -Os -fno-common # -mpcrel
WFLAGS    += -Wall -Wno-builtin-declaration-mismatch
CPPFLAGS  += -I$(TOPDIR)/FreeRTOS/portable/m68k-amiga \
	     -I$(TOPDIR)/include \
	     -I$(TOPDIR)
LDFLAGS   += -nostdlib
