        include 'hardware/custom.i'
        include 'hardware/cia.i'

        xref _start
        xdef _entry

        section '.init',code

; Boot data carries all necessary information for FreeRTOS startup,
; that is normally filled out by boot loader.
_bootdata:
        dc.l    _text           ; bd_entry
        dc.l    0               ; bd_vbr
        dc.w    0               ; bd_cpumodel : 68000
        dc.w    2               ; bd_nregions
        dc.l    _ebss, _eslow   ; slow memory
        dc.l    $400, _echip    ; chip memory (reserve exception vector)
        dc.l    0, 0            ; terminator 

; The ROM is located at $fc0000 but is mapped at $0 after reset shadowing RAM
_entry:
        lea     ciaa,a6
        move.b  #3,ciaddra(a6)  ; Set port A direction to output for /LED and OVL
        move.b  #0,ciapra(a6)   ; Disable OVL (Memory from $0 onwards available)

inithw:
        move.w  #$7fff,d0       ; Make sure DMA and interrupts are disabled
        lea     custom,a6
        move.w  d0,intena(a6)
        move.w  d0,intreq(a6)
        move.w  d0,dmacon(a6)

clrstack:                       ; Clear stack space
        lea     _stack,a0
        lea     _estack,a1
.loop   clr.l   (a0)+
        cmp.l   a1,a0
        blt     .loop

clrbss:                         ; Clear BSS section space
        lea     _bss,a0
        lea     _ebss,a1
.loop   clr.l   (a0)+
        cmp.l   a1,a0
        blt     .loop

copyrom:                        ; Copy TEXT & DATA sections to RAM
        lea     _rom,a0
        lea     _erom,a1
        lea     _text,a2
.loop   move.l  (a0)+,(a2)+
        cmp.l   a1,a0
        blt     .loop

doneinit:
        move.l  #_bootdata,-(sp)
        jsr     _start
        stop    #$2700

; vim: ft=asm68k:ts=8:sw=8:noet:
