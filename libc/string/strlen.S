; This code is covered by BSD license. It was taken from NetBSD
; common/lib/libc/arch/m68k/string/strlen.S and converted to Motorola syntax.

        xdef    strlen

        section '.text',code

strlen:
        move.l  4(sp),a0                ; string
        move.l  a0,d0
        not.l   d0
.Lslloop:
        tst.b   (a0)+                   ; null?
        bne     .Lslloop                ; no, keep going
        add.l   a0,d0
        rts

; vim: ft=asm68k:ts=8:sw=8:noet:
