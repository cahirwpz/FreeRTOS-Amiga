        xdef AmigaLvl1Handler
        xdef AmigaLvl2Handler
        xdef AmigaLvl3Handler
        xdef AmigaLvl4Handler
        xdef AmigaLvl5Handler
        xdef AmigaLvl6Handler
        xdef DummyInterruptHandler

        xref IntVec
        xref vPortYieldHandler
        xref xNeedRescheduleTask

INTREQR equ $dff01e
INTREQ  equ $dff09c

TBE     equ 0
DSKBLK  equ 1
SOFTINT equ 2
PORTS   equ 3
COPER   equ 4
VERTB   equ 5
BLIT    equ 6
AUD2    equ 7
AUD0    equ 8
AUD3    equ 9
AUD1    equ 10
RBF     equ 11
DSKSYNC equ 12
EXTER   equ 13

        section '.text',code

        macro ENTER
        movem.l d0-d1/a0-a1,-(sp)
        lea     IntVec+8*\1,a0
        moveq   #\1,d1
        endm

        macro HANDLE
        btst    d1,d0
        bne     _handle
        addq.l  #8,a0
        addq.w  #1,d1
        endm

        macro FINISH
        bra     _handle
        endm

; Level 1 Interrupt Autovector

AmigaLvl1Handler:
        ENTER   TBE
        move.w  INTREQR,d0

        HANDLE  TBE
        HANDLE  DSKBLK
        FINISH  ; SOFTINT

; Level 2 Interrupt Autovector

AmigaLvl2Handler:
        ENTER   PORTS
        FINISH

; Level 3 Interrupt Autovector

AmigaLvl3Handler:
        ENTER   COPER
        move.w  INTREQR,d0

        HANDLE  COPER
        HANDLE  VERTB
        FINISH  ; BLIT

; Level 4 Interrupt Autovector

AmigaLvl4Handler:
        ENTER   AUD2
        move.w  INTREQR,d0

        HANDLE  AUD2
        HANDLE  AUD0
        HANDLE  AUD3
        FINISH  ; AUD1

; Main part of interrupt handler

_handle:
        ; Make INTF_* mask, and clear pending interrupt.
        clr.w   d0
        bset    d1,d0
        move.w  d0,INTREQ

        ; Enter interrupt service routine.
        move.l  (a0)+,a1                ; IntVecEntry_t.code
        move.l  (a0)+,-(sp)             ; IntVecEntry_t.data
        jsr     (a1)
        addq.l  #4,sp

        ; Check if we need to reschedule a task - usually as a result waking
        ; up a higher priorty task while running interrupt service routine.
        ; Do it only when returning from interrupt context to task context.
        ; Let's not do the switch while returning from nested interrupt.
        move.w  16(sp),d0               ; SR from interrupted context
        and.w   #$0700,d0               ; was IPL set to 0 (task context) ?
        bne     _noswitch

        ; Turn off interrupts to prevent data races on xNeedRescheduleTask.
        or.w    #$0700,sr

        tst.w   xNeedRescheduleTask+2   ; check reschedule flag
        beq     _noswitch

        clr.w   xNeedRescheduleTask+2   ; clear reschedule flag
        movem.l (sp)+,d0-d1/a0-a1       ; restore registers
        jmp     vPortYieldHandler+4     ; force a task switch (skip or.w to sr)

_noswitch:
        movem.l (sp)+,d0-d1/a0-a1
        rte

; Level 5 Interrupt Autovector

AmigaLvl5Handler:
        ENTER   RBF
        move.w  INTREQR,d0

        HANDLE  RBF
        FINISH  ; DSKSYNC

; Level 6 Interrupt Autovector

AmigaLvl6Handler:
        ENTER   EXTER
        FINISH  ; EXTER

; Dummy handler

DummyInterruptHandler:
        rts

; vim: ft=asm68k ts=8 sw=8
