# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/strrchr.S

#include <asm.h>

ENTRY(strrchr)
	move.l	4(sp),a1	/* string */
	move.b	11(sp),d0	/* char to look for */
	sub.l	a0,a0		/* clear rindex pointer */
.Lrixloop:
	cmp.b	(a1),d0		/* found our char? */
	jne	.Lrixnope	/* no, check for null */
	move.l	a1,a0		/* yes, remember location */
.Lrixnope:
	tst.b	(a1)+		/* null? */
	jne	.Lrixloop	/* no, keep going */
	move.l	a0,d0		/* return value */
	rts
END(strrchr)

STRONG_ALIAS(rindex,strrchr)

# vim: ft=gas:ts=8:sw=8:noet:
