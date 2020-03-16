#include <asm.h>

#define portSAVE_CONTEXT()                                              \
        or.w    #0x0700,sr;             /* mask all interrupts */       \
        movem.l d0-a6,-(sp);            /* save D0-A6 registers */      \
        move.l  pxCurrentTCB,a0;        /* load current task pointer */ \
        move.l  sp,(a0);                /* save stack pointer */

#define portRESTORE_CONTEXT()                                           \
        move.l  pxCurrentTCB,a0;        /* load current task pointer */ \
        move.l  (a0),sp;                /* restore stack pointer */     \
        movem.l (sp)+,d0-a6;            /* restore D0-A6 registers */   \
        rte;                            /* restore SR and PC */

ENTRY(ulPortSetIPL)
        move.l  4(sp),d0
        move.w  sr,-(sp)
        move.w  sr,d1
        and.w   #0xf8ff,d1              /* old SR with IPL bits cleared */
        and.w   #0x0700,d0              /* argument with IPL bits left */
        or.w    d0,d1
        move.w  d1,sr                   /* set new value of IPL in SR */
        move.w  (sp)+,d0
        and.w   #0x0700,d0              /* return IPL bits of original SR */
        rts
END(ulPortSetIPL)

ENTRY(vPortYieldHandler)
        portSAVE_CONTEXT()
        jsr     vTaskSwitchContext
        portRESTORE_CONTEXT()
END(vPortYieldHandler)

ENTRY(vPortStartFirstTask)
        portRESTORE_CONTEXT()
END(vPortStartFirstTask)

# vim: ft=gas:ts=8:sw=8:noet
