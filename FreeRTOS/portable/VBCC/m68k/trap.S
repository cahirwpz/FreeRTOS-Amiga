        xdef _BadTrap
        xdef _BusErrTrap
        xdef _AddrErrTrap
        xdef _IllegalTrap
        xdef _ZeroDivTrap
        xdef _ChkInstTrap
        xdef _TrapvInstTrap
        xdef _PrivInstTrap
        xdef _TraceTrap
        xdef _FmtErrTrap
        xref _vPortTrapHandler

T_UNKNOWN   equ 0
T_BUSERR    equ 1
T_ADDRERR   equ 2
T_ILLINST   equ 3
T_ZERODIV   equ 4
T_CHKINST   equ 5
T_TRAPVINST equ 6
T_PRIVINST  equ 7
T_TRACE	    equ 8
T_FMTERR    equ 9

        section "AmigaTrap",text

        macro HANDLER
\1:
        move.w  #\2,-(sp)               ; save trap number
        bra     _EnterTrap
        endm

; List of trap handlers

        HANDLER _BadTrap,T_UNKNOWN
        HANDLER _BusErrTrap,T_BUSERR
        HANDLER _AddrErrTrap,T_ADDRERR
        HANDLER _IllegalTrap,T_ILLINST
        HANDLER _ZeroDivTrap,T_ZERODIV
        HANDLER _ChkInstTrap,T_CHKINST
        HANDLER _TrapvInstTrap,T_TRAPVINST
        HANDLER _PrivInstTrap,T_PRIVINST
        HANDLER _TraceTrap,T_TRACE
        HANDLER _FmtErrTrap,T_FMTERR

_EnterTrap:
        movem.l d0-a7,-(sp)             ; save all registers
        move.l  sp,-(sp)                ; pass trap frame as an argument
        jsr     _vPortTrapHandler
        stop    #$2700

; vim: ft=asm68k ts=8 sw=8
