        xdef _AmigaLvl1Handler
        xdef _AmigaLvl2Handler
        xdef _AmigaLvl3Handler
        xdef _AmigaLvl4Handler
        xdef _AmigaLvl5Handler
        xdef _AmigaLvl6Handler
        xdef _DummyInterruptHandler
        xref _IntVec

INTREQR equ $dff01e

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

        section "AmigaIntr",text

        macro ENTER
        movem.l d0-d1/a0-a1,-(sp)
        lea     _IntVec+4*\1,a0
        endm

; Level 1 Interrupt Autovector

_AmigaLvl1Handler:
        ENTER   TBE
        move.w  INTREQR,d0

_tbe    
        btst    #TBE,d0
        bne     _handle

_dskblk
        addq.l  #4,a0
        btst    #DSKBLK,d0
        bne     _handle

_softint
        addq.l  #4,a0
        bra     _handle

; Level 2 Interrupt Autovector

_AmigaLvl2Handler:
        ENTER   PORTS
        bra     _handle

; Level 3 Interrupt Autovector

_AmigaLvl3Handler:
        ENTER   COPER
        move.w  INTREQR,d0

_coper  
        btst    #COPER,d0
        bne     _handle

_vertb  
        addq.l  #4,a0
        btst    #VERTB,d0
        bne     _handle

_blit
        addq.l  #4,a0
        bra     _handle

; Level 4 Interrupt Autovector

_AmigaLvl4Handler:
        ENTER   AUD2
        move.w  INTREQR,d0

_aud2
        btst    #AUD2,d0
        bne     _handle

_aud0
        addq.l  #4,a0
        btst    #AUD0,d0
        bne     _handle

_aud3
        addq.l  #4,a0
        btst    #AUD3,d0
        bne     _handle

_aud1
        addq.l  #4,a0
        bra     _handle

; Level 5 Interrupt Autovector

_AmigaLvl5Handler:
        ENTER   RBF
        move.w  INTREQR,d0

_rbf
        btst    #RBF,d0
        bne     _handle

_dsksync
        addq.l  #4,a0
        bra     _handle

; Main part of interrupt handler

_handle:
        move.l  (a0),a0
        jsr     (a0)
        movem.l (sp)+,d0-d1/a0-a1
        rte

; Level 6 Interrupt Autovector

_AmigaLvl6Handler:
        ENTER   EXTER
        bra     _handle

; Dummy handler

_DummyInterruptHandler:
        rts

; vim: ft=asm68k ts=8 sw=8
