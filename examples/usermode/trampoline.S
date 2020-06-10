#include <asm.h>

ENTRY(EnterUserMode)
        move.l  4(sp),a0        /* program counter */
        move.l  8(sp),a1        /* stack pointer */
        move.l  a1,usp          /* set up stack pointer */
        move.l  a0,-(sp)        /* usermode PC */
        clr.w   -(sp)           /* usermode SR=0 */
        rte                     /* finally, enter user mode */
END(EnterUserMode)

# vim: ft=gas:ts=8:sw=8:noet
