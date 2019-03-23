; This code is covered by BSD license. It was taken from NetBSD
; common/lib/libc/arch/m68k/string/strcat.S and converted to Motorola syntax.

        XDEF    _strcat

        SECTION "strcat",CODE
_strcat:
	move.l	8(sp),a0		; a0 = fromaddr
	move.l	4(sp),d0		; return value is toaddr
	move.l	d0,a1			; a1 = toaddr
.Lslloop:
	tst.b	(a1)+			; null?
	bne	.Lslloop		; no, keep going
	subq.l	#1,a1
.Lscloop:
	move.b	(a0)+,(a1)+		; copy a byte
	bne	.Lscloop		; copied non-null, keep going
	rts

; vim: ft=asm68k:ts=8:sw=8:noet:
