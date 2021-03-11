#include <asm.h>
#include <custom_regdef.h>
#include <interrupt.h>

.macro  FIRST   intb
        movem.l d0-d1/a0-a1,-(sp)
        moveq   #\intb,d1
        move.w  custom+intreqr,d0
        and.w   custom+intenar,d0
        btst    d1,d0
        jne     EnterIntr
.endm

.macro  NEXT    irq
        addq.w  #1,d1
        btst    d1,d0
        jne     EnterIntr
.endm

.macro  FINISH
        jra     LeaveIntr
.endm

# Level 1 Interrupt Autovector

ENTRY(AmigaLvl1Handler)
        FIRST   INTB_TBE
        NEXT    INTB_DSKBLK
        NEXT    INTB_SOFTINT
        FINISH
END(AmigaLvl1Handler)

# Level 2 Interrupt Autovector

ENTRY(AmigaLvl2Handler)
        FIRST   INTB_PORTS
        FINISH
END(AmigaLvl2Handler)

# Level 3 Interrupt Autovector

ENTRY(AmigaLvl3Handler)
        FIRST   INTB_COPER
        NEXT    INTB_VERTB
        NEXT    INTB_BLIT
        FINISH
END(AmigaLvl3Handler)

# Level 6 Interrupt Autovector

ENTRY(AmigaLvl6Handler)
        FIRST   INTB_EXTER
        FINISH
END(AmigaLvl6Handler)

# Main part of interrupt handler

ENTRY(EnterIntr)
        /* Make INTF_* mask, and clear pending interrupt. */
        clr.w   d0
        bset    d1,d0
        move.w  d0,custom+intreq

        lea     IntVec,a0
        lsl.w   #3,d1
        add.w   d1,a0

        /* Enter interrupt service routine. */
        move.l  (a0)+,a1                /* IntVecEntry_t.code */
        move.l  (a0)+,-(sp)             /* IntVecEntry_t.data */
        jsr     (a1)
        addq.l  #4,sp
END(EnterIntr)

ENTRY(LeaveIntr)
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
END(LeaveIntr)

# Level 4 Interrupt Autovector

ENTRY(AmigaLvl4Handler)
        FIRST   INTB_AUD0
        NEXT    INTB_AUD1
        NEXT    INTB_AUD2
        NEXT    INTB_AUD3
        FINISH
END(AmigaLvl4Handler)

# Level 5 Interrupt Autovector

ENTRY(AmigaLvl5Handler)
        FIRST   INTB_RBF
        NEXT    INTB_DSKSYNC
        FINISH
END(AmigaLvl5Handler)

# Dummy handler

ENTRY(DummyInterruptHandler)
        rts
END(DummyInterruptHandler)

# vim: ft=gas:ts=8:sw=8:noet:
