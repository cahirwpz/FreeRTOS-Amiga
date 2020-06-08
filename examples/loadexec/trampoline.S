#include <asm.h>

/* After EnterUserMode gets executed current context is left on stack.
 * It's placed just after TrapFrame when we enter vPortTrapHandler. */

ENTRY(EnterUserMode)
        move.l  4(sp),a0                /* program counter */
        move.l  8(sp),a1                /* stack pointer */
        movem.l d2-d7/a2-a6,-(sp)
        move.l  a1,usp                  /* set up stack pointer */
        move.l  a0,-(sp)                /* usermode PC */
        clr.w   -(sp)                   /* usermode SR=0 */
        rte                             /* finally, enter user mode */
END(EnterUserMode)

ENTRY(DropUserMode)
        movem.l (sp)+,d2-d7/a2-a6
        rts
END(DropUserMode)

# vim: ft=gas:ts=8:sw=8:noet
