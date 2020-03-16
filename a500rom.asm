        include 'hardware/custom.i'
        include 'hardware/cia.i'

ChipStart equ $400
ChipEnd   equ $80000
SlowStart equ $c00000
SlowEnd   equ $c80000

; custom chips addresses

custom  equ     $dff000
ciaa    equ     $bfe001
ciab    equ     $bfd000

; A500 kickstart begin address

        org     $fc0000

Kickstart:
        dc.l    $400             ; Initial SP
        dc.l    Entry            ; Initial PC

; The ROM is located at $fc0000 but is mapped at $0 after reset shadowing RAM
Entry:
        lea     ciaa,a6
        move.b  #3,ciaddra(a6)  ; Set port A direction to output for /LED and OVL
        move.b  #0,ciapra(a6)   ; Disable OVL (Memory from $0 onwards available)

InitHW:
        move.w  #$7fff,d0       ; Make sure DMA and interrupts are disabled
        lea     custom,a6
        move.w  d0,intena(a6)
        move.w  d0,intreq(a6)
        move.w  d0,dmacon(a6)

Setup:
        move.l  #(HunkFileEnd-HunkFile),d2
        move.l  #HunkFile,d3
        move.l  #BD_SIZE+3*MR_SIZE,a3
        sub.l   a3,sp
        move.l  sp,a6

        clr.l   BD_ENTRY(a6)
        clr.l   BD_VBR(a6)
        clr.w   BD_CPUMODEL(a6)
        move.w  #2,BD_NREGIONS(a6)

        lea     BD_REGION(a6),a0
        move.l  #SlowStart,(a0)+
        move.l  #SlowEnd,(a0)+
        move.l  #ChipStart,(a0)+
        move.l  #ChipEnd,(a0)+
        clr.l   (a0)+
        clr.l   (a0)+

ROM = 1

        include '$(TOPDIR)/bootloader.asm'

HunkFile
        incbin  '$(PROGRAM).exe'
HunkFileEnd

        org     $fffff0

        dc.w    $4718, $4819, $4f1a, $531b
        dc.w    $541c, $4f1d, $571e, $4e1f

; vim: ft=asm68k:ts=8:sw=8:noet:
