#include <asm.h>

ENTRY(EnterUserMode)
        move.l  4(sp),a0        /* UserCtx_t pointer */
        move.l  (a0)+,-(sp)     /* push user PC */
        move.l  (a0)+,a1        /* read user stack pointer */
        move.l  a1,usp          /* set up user stack pointer register */
        movem.l (a0)+,d0-a6     /* restore all user visible registers */
        clr.w   -(sp)           /* push user SR=0 */
        rte                     /* finally, enter user mode */
END(EnterUserMode)

# vim: ft=gas:ts=8:sw=8:noet
