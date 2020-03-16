# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/strlen.S

#include <asm.h>

ENTRY(strlen)
        move.l  4(sp),a0        /* string */
        move.l  a0,d0
        not.l   d0
.Lslloop:
        tst.b   (a0)+           /* null? */
        jne     .Lslloop	/* no, keep going */
        add.l   a0,d0
        rts
END(strlen)

# vim:ft=gas:ts=8:sw=8:noet:
