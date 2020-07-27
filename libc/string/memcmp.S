# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/memcmp.S

#include <asm.h>

/*
 * This is probably not the best we can do, but it is still 2-10 times
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *	- longword align when possible (only on the 68020)
 *	- use nested DBcc instructions or use one and limit size to 64K
 */
ENTRY(memcmp)
	move.l	4(sp),a0	/* string 1 */
	move.l	8(sp),a1	/* string 2 */
	move.l	12(sp),d0	/* length */
	jeq	.Lbcdone	/* if zero, nothing to do */
	move.l	a0,d1
	btst	#0,d1		/* string 1 address odd? */
	jeq	.Lbceven	/* no, skip alignment */
	cmp.b   (a0)+,(a1)+	/* yes, compare a byte */
	jne	.Lbcnoteq       /* not equal, return non-zero */
	subq.l	#1,d0		/* adjust count */
	jeq	.Lbcdone	/* count 0, reutrn zero */
.Lbceven:
	move.l	a1,d1
	btst	#0,d1		/* string 2 address odd? */
        jne	.Lbcbloop	/* yes, no hope for alignment, compare bytes */
	move.l	d0,d1		/* no, both even */
	lsr.l	#2,d1		/* convert count to longword count */
	jeq	.Lbcbloop	/* count 0, skip longword loop */
.Lbclloop:
	cmp.l   (a0)+,(a1)+	/* compare a longword */
	jne	.Lbcnoteql	/* not equal, return non-zero */
	subq.l	#1,d1		/* adjust count */
	jne	.Lbclloop	/* still more, keep comparing */
	and.l	#3,d0		/* what remains */
	jeq	.Lbcdone	/* nothing, all done */
.Lbcbloop:
	cmp.b   (a0)+,(a1)+	/* compare a byte */
	jne	.Lbcnoteq	/* not equal, return non-zero */
	subq.l	#1,d0		/* adjust count */
	jne	.Lbcbloop	/* still more, keep going */
	rts
.Lbcnoteql:
	subq.l	#4,a0
	subq.l	#4,a1
	move.l	#4,d0
	jra	.Lbcbloop
.Lbcnoteq:
	clr.l	d0
	clr.l	d1
	move.b	-(a0),d0
	move.b	-(a1),d1
	sub.l	d1,d0
.Lbcdone:
	rts
END(memcmp)

# vim: ft=gas:ts=8:sw=8:noet:
