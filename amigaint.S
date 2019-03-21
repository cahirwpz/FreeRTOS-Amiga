        xdef _AmigaLvl1Handler
        xdef _AmigaLvl2Handler
        xdef _AmigaLvl3Handler
        xdef _AmigaLvl4Handler
        xdef _AmigaLvl5Handler
        xdef _AmigaLvl6Handler
        xdef _DummyInterruptHandler
        xdef _IntVec

INTREQR_HI equ $dff01e
INTREQR_LO equ $dff01f

TBE     equ 0
DSKBLK  equ 1
SOFTINT equ 2
PORTS   equ 3
COPER   equ 4
VERTB   equ 5
BLIT    equ 6
AUD2    equ 7
AUD0    equ 0
AUD3    equ 1
AUD1    equ 2
RBF     equ 3
DSKSYNC equ 4
EXTER   equ 5

        section "AmigaInt",text

        macro JMPVEC
        dc.w $4ef9
\1      dc.l _rte
	endm

; Level 1 Interrupt Autovector

_AmigaLvl1Handler:
_tbe    
        btst.b  #TBE,INTREQR_LO
        beq     _dskblk
        JMPVEC  tbe

_dskblk
        btst.b  #DSKBLK,INTREQR_LO
        beq     _softint
        JMPVEC  dskblk

_softint
        btst.b  #SOFTINT,INTREQR_LO
        beq     _rte
        JMPVEC  softint

; Level 2 Interrupt Autovector

_AmigaLvl2Handler:
_ports  
        btst.b  #PORTS,INTREQR_LO
        beq     _rte
        JMPVEC  ports

; Level 3 Interrupt Autovector

_AmigaLvl3Handler:
_coper
        btst.b  #COPER,INTREQR_LO
        beq     _vertb
        JMPVEC  coper

_vertb
        btst.b  #VERTB,INTREQR_LO
        beq     _blit
        JMPVEC  vertb

_blit   btst.b  #BLIT,INTREQR_LO
        beq     _rte
        JMPVEC  blit

; Dummy handler

_DummyInterruptHandler:
_rte:
        rte

; Level 4 Interrupt Autovector

_AmigaLvl4Handler:
_aud2
        btst.b  #AUD2,INTREQR_LO
        beq     _aud0
        JMPVEC  aud2

_aud0
        btst.b  #AUD0,INTREQR_HI
        beq     _aud3
        JMPVEC  aud0

_aud3
        btst.b  #AUD3,INTREQR_HI
        beq     _aud1
        JMPVEC  aud3

_aud1
        btst.b  #AUD1,INTREQR_HI
        beq     _rte
        JMPVEC  aud1

; Level 5 Interrupt Autovector

_AmigaLvl5Handler:
_rbf
        btst.b  #RBF,INTREQR_HI
        beq     _dsksync
        JMPVEC  rbf

_dsksync
        btst.b  #RBF,INTREQR_HI
        beq     _rte
        JMPVEC  dsksync

; Level 6 Interrupt Autovector

_AmigaLvl6Handler:
_exter
        btst.b  #RBF,INTREQR_HI
        beq     _rte
        JMPVEC  exter

        section "AmigaInt",data

_IntVec:
        dc.l    tbe
        dc.l    dskblk
        dc.l    softint
        dc.l    ports
        dc.l    coper
        dc.l    vertb
        dc.l    blit
        dc.l    aud2
        dc.l    aud0
        dc.l    aud3
        dc.l    aud1
        dc.l    rbf
        dc.l    dsksync
        dc.l    exter

; vim: ft=asm68k ts=8 sw=8
