# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/strcmp.S

#include <asm.h>

ENTRY(strcmp)
	move.l	4(sp),a0
	move.l	8(sp),a1
.L1:				/* unrolled by 4 for 680[23]0's */
	move.b  (a0)+,d1
	jeq	.L2
	sub.b   (a1)+,d1
	jne	.L3

	move.b  (a0)+,d1
	jeq	.L2
	sub.b   (a1)+,d1
	jne	.L3

	move.b  (a0)+,d1
	jeq	.L2
	sub.b   (a1)+,d1
	jne	.L3

	move.b  (a0)+,d1
	jeq	.L2
	sub.b   (a1)+,d1
	jeq	.L1
.L3:
	scs	d0
        ext.w   d0
        ext.l   d0
	move.b	d1,d0
	rts
.L2:
        moveq.l	#0,d0
	move.b	(a1),d0
	neg.l	d0
	rts
END(strcmp)

# vim: ft=gas:ts=8:sw=8:noet:
