#pragma once

/* Based on FreeBSD's <sys/ioccom.h> header file. */

#include <sys/types.h>

/*
 * Ioctl's have the command encoded in the lower word, and the size of
 * any in or out parameters in the upper word.  The high 3 bits of the
 * upper word are used to encode the in/out status of the parameter.
 *
 *   31 29 28                     16 15            8 7             0
 *  +---------------------------------------------------------------+
 *  | I/O | Parameter Length        | Command Group | Command       |
 *  +---------------------------------------------------------------+
 */
#define IOCPARM_SHIFT 13 /* number of bits for ioctl size */
#define IOCPARM_MASK ((1 << IOCPARM_SHIFT) - 1) /* parameter length mask */
#define IOCPARM_LEN(x) (((x) >> 16) & IOCPARM_MASK)
#define IOCBASECMD(x) ((x) & ~(IOCPARM_MASK << 16))
#define IOCGROUP(x) (((x) >> 8) & 0xff)

#define IOCPARM_MAX (1 << IOCPARM_SHIFT) /* max size of ioctl */

#define IOC_VOID 0x20000000UL        /* no parameters */
#define IOC_OUT 0x40000000UL         /* copy out parameters */
#define IOC_IN 0x80000000UL          /* copy in parameters */
#define IOC_INOUT (IOC_IN | IOC_OUT) /* copy parameters in and out */
#define IOC_DIRMASK (IOC_VOID | IOC_OUT | IOC_IN) /* mask for IN/OUT/VOID */

#define _IOC(inout, group, num, len)                                           \
  ((u_long)((inout) | (((len)&IOCPARM_MASK) << 16) | ((group) << 8) | (num)))
#define _IO(g, n) _IOC(IOC_VOID, (g), (n), 0)
#define _IOR(g, n, t) _IOC(IOC_OUT, (g), (n), sizeof(t))
#define _IOW(g, n, t) _IOC(IOC_IN, (g), (n), sizeof(t))
#define _IOWR(g, n, t) _IOC(IOC_INOUT, (g), (n), sizeof(t))

#ifdef _USERSPACE

int ioctl(int, u_long, ...);

#endif
