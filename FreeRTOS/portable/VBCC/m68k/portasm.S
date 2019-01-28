; FreeRTOS Kernel V10.2.0
; Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
;
; Permission is hereby granted, free of charge, to any person obtaining a copy of
; this software and associated documentation files (the "Software"), to deal in
; the Software without restriction, including without limitation the rights to
; use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
; the Software, and to permit persons to whom the Software is furnished to do so,
; subject to the following conditions:
;
; The above copyright notice and this permission notice shall be included in all
; copies or substantial portions of the Software.
;
; THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
; IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
; FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
; COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
; IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
; CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

; Purpose: Lowest level routines for all ColdFire processors.
;
; Notes:
; 
; ulPortSetIPL() and mcf5xxx_wr_cacr() copied with permission from FreeScale
; supplied source files.

        xdef ulPortSetIPL
        xdef vPortStartFirstTask

        section "portasm.S",text

        macro portSAVE_CONTEXT
	lea.l   -60(sp),sp
	movem.l	d0-a6,(sp)
	move.l	pxCurrentTCB,a0
	move.l	sp,(a0)
	endm

        macro portRESTORE_CONTEXT
	move.l	pxCurrentTCB,a0
	move.l	(a0),sp
	movem.l	(sp),d0-a6
	lea.l	60(sp),sp
	rte
	endm

;********************************************************************
; This routines changes the IPL to the value passed into the routine.
; It also returns the old IPL value back.
; Calling convention from C:
;   old_ipl = asm_set_ipl(new_ipl);
; For the Diab Data C compiler, it passes return value thru D0.
; Note that only the least significant three bits of the passed
; value are used.

ulPortSetIPL:
    link    a6,#-8
    movem.l d6-d7,(sp)

    move.w  sr,d7               ; current sr

    move.l  d7,d0               ; prepare return value
    andi.l  #$0700,d0           ; mask out ipl
    lsr.l   #8,d0               ; ipl

    move.l  8(a6),d6            ; get argument
    andi.l  #7,d6               ; least significant three bits
    lsl.l   #8,d6               ; move over to make mask

    andi.l  #$f8ff,d7           ; zero out current ipl
    or.l    d6,d7               ; place new ipl in sr
    move.w  d7,sr

    movem.l (sp),d6-d7
    lea     8(sp),sp
    unlk    a6
    rts

;********************************************************************

vPortStartFirstTask:
	portRESTORE_CONTEXT

; vim: ft=asm68k
