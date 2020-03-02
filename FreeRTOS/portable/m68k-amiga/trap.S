        xdef BadTrap
        xdef BusErrTrap
        xdef AddrErrTrap
        xdef IllegalTrap
        xdef ZeroDivTrap
        xdef ChkInstTrap
        xdef TrapvInstTrap
        xdef PrivInstTrap
        xdef TraceTrap
        xdef FmtErrTrap
        xdef TrapInstTrap
        xref vPortTrapHandler

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
T_TRAPINST  equ 10

        section '.text',code

        macro HANDLER
\1:
        move.w  #\2,-(sp)               ; save trap number
        bra     EnterTrap
        endm

; List of trap handlers

        HANDLER BadTrap,T_UNKNOWN
        HANDLER BusErrTrap,T_BUSERR
        HANDLER AddrErrTrap,T_ADDRERR
        HANDLER IllegalTrap,T_ILLINST
        HANDLER ZeroDivTrap,T_ZERODIV
        HANDLER ChkInstTrap,T_CHKINST
        HANDLER TrapvInstTrap,T_TRAPVINST
        HANDLER PrivInstTrap,T_PRIVINST
        HANDLER TraceTrap,T_TRACE
        HANDLER FmtErrTrap,T_FMTERR
        HANDLER TrapInstTrap, T_TRAPINST

EnterTrap:
        movem.l d0-a7,-(sp)             ; save all registers
        move.l  sp,-(sp)                ; pass trap frame as an argument
        jsr     vPortTrapHandler
        addq.l  #4,sp
        movem.l (sp)+,d0-a7             ; restore all registers
        addq.l  #2,sp                   ; pop trap number
        rte

; vim: ft=asm68k ts=8 sw=8
