#include <asm.h>
#include "syscall.h"

/* WARNING! Since executable loader is very primitive _start procedure
 * MUST BE first one in .text section of the program! */
ENTRY(_start)
        jsr     main
        move.l  d0,d1
        moveq.l #SYS_exit,d0
        trap    #1
END(_start)

# vim: ft=gas:ts=8:sw=8:noet
