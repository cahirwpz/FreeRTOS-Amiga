#include <asm.h>
#include <trap.h>

#define HANDLER(name,trap)                                      \
ENTRY(name);                                                    \
        move.w  __IMMEDIATE trap,-(sp); /* save trap number */  \
        jra     EnterTrap;                                      \
END(name)

# List of trap handlers

HANDLER(BadTrap,T_UNKNOWN)
HANDLER(BusErrTrap,T_BUSERR)
HANDLER(AddrErrTrap,T_ADDRERR)
HANDLER(IllegalTrap,T_ILLINST)
HANDLER(ZeroDivTrap,T_ZERODIV)
HANDLER(ChkInstTrap,T_CHKINST)
HANDLER(TrapvInstTrap,T_TRAPVINST)
HANDLER(PrivInstTrap,T_PRIVINST)
HANDLER(TraceTrap,T_TRACE)
HANDLER(FmtErrTrap,T_FMTERR)
HANDLER(TrapInstTrap,T_TRAPINST)

ENTRY(EnterTrap)
        movem.l d0-a7,-(sp)     /* save all registers */
        move.l  sp,-(sp)        /* pass trap frame as an argument */
        jsr     vPortTrapHandler
        addq.l  #4,sp
        movem.l (sp)+,d0-a7     /* restore all registers */
        addq.l  #2,sp           /* pop trap number */
        rte
END(EnterTrap)

# vim: ft=gas:ts=8:sw=8:noet:
