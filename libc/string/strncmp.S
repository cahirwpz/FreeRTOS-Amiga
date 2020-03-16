# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/strncmp.S

#include <asm.h>

ENTRY(strncmp)
	move.l	12(sp),d0
	jeq	.L4
	move.l	4(sp),a0
	move.l	8(sp),a1
.L1:				/* unroll by 4 for m680[23]0's */
	move.b  (a0)+,d1
	jeq	.L2
	sub.b   (a1)+,d1
	jne	.L3
	subq.l	#1,d0
	jeq	.L4

	move.b  (a0)+,d1
	jeq	.L2
	sub.b   (a1)+,d1
	jne	.L3
	subq.l	#1,d0
	jeq	.L4

	move.b  (a0)+,d1
	jeq	.L2
	sub.b   (a1)+,d1
	jne	.L3
	subq.l	#1,d0
	jeq	.L4

	move.b  (a0)+,d1
	jeq	.L2
	sub.b   (a1)+,d1
	jne	.L3
	subq.l	#1,d0
	jne	.L1
.L4:
        rts
.L2:
        sub.b   (a1),d1
.L3:	
	scs	d0
        ext.w   d0
        ext.l   d0
	move.b	d1,d0
	rts
END(strncmp)

# vim: ft=gas:ts=8:sw=8:noet:
