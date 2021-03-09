#include <asm.h>
#include <custom_regdef.h>
#include <interrupt.h>

#define ISR(name,num)                           \
ENTRY(name);                                    \
        movem.l d0-d1/a0-a1,-(sp);              \
        lea     IntVec+8*num,a0;                \
        moveq   __IMMEDIATE num,d1

#define HANDLE                                  \
        btst    d1,d0;                          \
        jne     _handle;                        \
        addq.l  #8,a0;                          \
        addq.w  #1,d1

#define FINISH                                  \
        jra     _handle

# Level 1 Interrupt Autovector

ISR(AmigaLvl1Handler,INTB_TBE)
        move.w  custom+intreqr,d0

        HANDLE  /* TBE */
        HANDLE  /* DSKBLK */
        FINISH  /* SOFTINT */
END(AmigaLvl1Handler)

# Level 2 Interrupt Autovector

ISR(AmigaLvl2Handler,INTB_PORTS)
        FINISH  /* PORTS */
END(AmigaLvl2Handler)

# Level 3 Interrupt Autovector

ISR(AmigaLvl3Handler,INTB_COPER)
        move.w  custom+intreqr,d0

        HANDLE  /* COPER */
        HANDLE  /* VERTB */
        FINISH  /* BLIT */
END(AmigaLvl3Handler)

# Level 4 Interrupt Autovector

ISR(AmigaLvl4Handler,INTB_AUD0)
        move.w  custom+intreqr,d0

        HANDLE  /* AUD0 */
        HANDLE  /* AUD1 */
        HANDLE  /* AUD2 */
        /* FINISH AUD3 */
END(AmigaLvl4Handler)

# Main part of interrupt handler

_handle:
        /* Make INTF_* mask, and clear pending interrupt. */
        clr.w   d0
        bset    d1,d0
        move.w  d0,custom+intreq

        /* Enter interrupt service routine. */
        move.l  (a0)+,a1                /* IntVecEntry_t.code */
        move.l  (a0)+,-(sp)             /* IntVecEntry_t.data */
        jsr     (a1)
        addq.l  #4,sp

        /*
         * Check if we need to reschedule a task - usually as a result waking
         * up a higher priorty task while running interrupt service routine.
         * Do it only when returning from interrupt context to task context.
         * Let's not do the switch while returning from nested interrupt.
         */
        move.w  16(sp),d0               /* SR from interrupted context */
        and.w   #0x0700,d0              /* was IPL set to 0 (task context) ? */
        jne     .Lnoswitch

        /* Turn off interrupts to prevent data races on xNeedRescheduleTask. */
        or.w    #0x0700,sr

        tst.w   xNeedRescheduleTask+2   /* check reschedule flag */
        jeq     .Lnoswitch

        clr.w   xNeedRescheduleTask+2   /* clear reschedule flag */
        movem.l (sp)+,d0-d1/a0-a1       /* restore registers */
        jmp     vPortYieldHandler+4     /* force a task switch (skip or.w to sr) */

.Lnoswitch:
        movem.l (sp)+,d0-d1/a0-a1
        rte

# Level 5 Interrupt Autovector

ISR(AmigaLvl5Handler,INTB_RBF)
        move.w  custom+intreqr,d0

        HANDLE  /* RBF *
        FINISH  /* DSKSYNC */
END(AmigaLvl5Handler)

# Level 6 Interrupt Autovector

ISR(AmigaLvl6Handler,INTB_EXTER)
        FINISH  /* EXTER */
END(AmigaLvl6Handler)

# Dummy handler

ENTRY(DummyInterruptHandler)
        rts
END(DummyInterruptHandler)

# vim: ft=gas:ts=8:sw=8:noet:
