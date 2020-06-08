#include <asm.h>

ENTRY(longjmp)
        move.l  4(sp),a0                /* jmp_buf */
        move.l  8(sp),d0                /* return val */
        moveml  (a0)+,d2-d7/a2-a7       /* restore caller registers */
        move.l  (a0),(sp)               /* restore saved pc */
        rts
END(longjmp)

# vim: ft=gas:ts=8:sw=8:noet:
