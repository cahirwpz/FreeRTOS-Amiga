# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/strchr.S

#include <asm.h>

ENTRY(strchr)
	move.l	4(sp),a0	/* string */
	move.b	11(sp),d0	/* char to look for */
.Lixloop:
	cmp.b	(a0),d0		/* found our char? */
	jeq	.Lixfound	/* yes, break out */
	tst.b	(a0)+		/* null? */
	jne	.Lixloop	/* no, keep going */
	moveq	#0,d0		/* not found, return null */
	rts
.Lixfound:
	move.l	a0,d0		/* found, return pointer */
	rts
END(strchr)

STRONG_ALIAS(index,strchr)

# vim: ft=gas:ts=8:sw=8:noet:
