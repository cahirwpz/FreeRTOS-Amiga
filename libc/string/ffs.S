# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/ffs.S

#include <asm.h>

ENTRY(ffs)
	move.l	4(sp),d0
	jeq	.L2
	moveq.l	#31,d1
.L1:	
	lsr.l	#1,d0
	dbcs	d1,.L1
	moveq.l	#32,d0
	sub.l	d1,d0
.L2:
	rts
END(ffs)

# vim: ft=gas:ts=8:sw=8:noet:
