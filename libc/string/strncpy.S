# This code is covered by BSD license. It was taken from NetBSD
# common/lib/libc/arch/m68k/string/strncpy.S

#include <asm.h>

ENTRY(strncpy)
	move.l	4(sp),d0	/* return value is toaddr */
	move.l	12(sp),d1	/* count */
	jeq	.Lscdone	/* nothing to do */
	move.l	8(sp),a0	/* a0 = fromaddr */
	move.l	d0,a1		/* a1 = toaddr */
	subq.l	#1,d1
.Lscloop:
	move.b	(a0)+,(a1)+	/* copy a byte */
	dbeq	d1,.Lscloop	/* loop through low word of d1 */
	jeq	.Lscpadding	/* copied null, padding if necessary */
	clr.w	d1		/* clear low word of d1 */
	subq.l	#1,d1		/* adjust count for long copies */
	jcc	.Lscloop	/* more room, keep going */
.Lscdone:
	rts

.Lscploop:
	clr.b	(a1)+		/* clear a byte */
	dbra	d1,.Lscploop	/* keep going */
	clr.w	d1
.Lscpadding:
	subq.l	#1,d1
	jcc	.Lscploop
	jra	.Lscdone
END(strncpy)

# vim: ft=gas:ts=8:sw=8:noet:
