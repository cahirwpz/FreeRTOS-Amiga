# Reliable transfers may only happen 3 or more E-cycles basis.
# http://lallafa.de/blog/2015/09/amiga-parallel-port-how-fast-can-you-go/

#include <asm.h>
#include <cia_regdef.h>

ENTRY(BootConsPutChar)
        move.l  4(sp),d0
        lea     ciab,a0                 /* [a0] ciab, ciaa = ciab + $1001 */

.Loop:  btst    #CIAB_PRTRBUSY,(a0)     /* is printer buffer empty ? */
        jne     .Loop

        move.b  #255,ciaddrb+0x1001(a0) /* set direction to out */
        move.b  d0,ciaprb+0x1001(a0)	/* write the data */
        tst.b   (a0)                    /* waste 2 E-cycles */
        tst.b   (a0)
        rts
END(BootConsPutChar)

# vim: ft=gas:ts=8:sw=8:noet:
