ASFLAGS   += -Wa,--register-prefix-optional -Wa,--bitwise-or
VASMFLAGS += -m68010 -quiet -I$(PREFIX)/m68k-amigaos/include
CFLAGS    += -fno-builtin -nostdinc -nostdlib -ffreestanding
CFLAGS    += -std=gnu11 $(OFLAGS) $(WFLAGS)
OFLAGS    += -m68010 -Wall -Wextra -fomit-frame-pointer -Os -fno-common # -mpcrel
WFLAGS    += -Wall -Werror -Wno-builtin-declaration-mismatch
CPPFLAGS  += -I$(TOPDIR)/FreeRTOS-Plus/include \
	     -I$(TOPDIR)/kernel/include \
	     -I$(TOPDIR)/drivers/include \
	     -I$(TOPDIR)/libc/include \
	     -I$(TOPDIR)
LDFLAGS   += -nostdlib
