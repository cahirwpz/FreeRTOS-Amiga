PORT_DIR = portable/m68k-amiga

ASFLAGS   += -Wa,--register-prefix-optional -Wa,--bitwise-or
VASMFLAGS += -m68010 -quiet -I$(PREFIX)/m68k-amigaos/include
CFLAGS    += -fno-builtin -nostdinc -nostdlib -ffreestanding
CFLAGS    += -std=gnu11 $(OFLAGS) $(WFLAGS)
OFLAGS    += -m68010 -Wall -Wextra -fomit-frame-pointer -Os -fno-common # -mpcrel
WFLAGS    += -Wall -Werror -Wno-builtin-declaration-mismatch
CPPFLAGS  += -I$(TOPDIR)/FreeRTOS/$(PORT_DIR) \
             -I$(TOPDIR)/FreeRTOS-Plus/include \
	     -I$(TOPDIR)/FreeRTOS-Plus/$(PORT_DIR) \
	     -I$(TOPDIR)/libc/include \
	     -I$(TOPDIR)/include \
	     -I$(TOPDIR)
LDFLAGS   += -nostdlib
