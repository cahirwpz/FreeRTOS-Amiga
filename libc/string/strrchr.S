; This code is covered by BSD license. It was taken from NetBSD
; common/lib/libc/arch/m68k/string/strrchr.S and converted to Motorola syntax.

        XDEF    _rindex
        XDEF    _strrchr

        SECTION "strrchr",CODE

_rindex:
_strrchr:
	move.l	4(sp),a1		; string
	move.b	11(sp),d0		; char to look for
	sub.l	a0,a0			; clear rindex pointer
.Lrixloop:
	cmp.b	(a1),d0		        ; found our char?
	bne	.Lrixnope		; no, check for null
	move.l	a1,a0			; yes, remember location
.Lrixnope:
	tst.b	(a1)+			; null?
	bne	.Lrixloop		; no, keep going
	move.l	a0,d0			; return value
	rts

; vim: ft=asm68k:ts=8:sw=8:noet:
