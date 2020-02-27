#ifndef _SPRITE_H_
#define _SPRITE_H_

#include <stdint.h>

/*
 * Sprites are described here:
 * http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node00AE.html
 */

typedef uint16_t sprite_t[][2];

/*
 * SPRxPOS:
 *  Bits 15-8 contain the low 8 bits of VSTART (y)
 *  Bits 7-0 contain the high 8 bits of HSTART (x)
 */
#define SPRPOS(x, y) (((y) << 8) | (((x) >> 1) & 255))

/*
 * SPRxCTL:
 *  Bits 15-8       The low eight bits of VSTOP
 *  Bit 7           (Used in attachment)
 *  Bits 6-3        Unused (make zero)
 *  Bit 2           The VSTART high bit
 *  Bit 1           The VSTOP high bit
 *  Bit 0           The HSTART low bit
 */
#define SPRCTL(x, y, a, h)                                                     \
  ((((x) + (h) + 1) << 8) |                                                    \
   (((a) & 1) << 7) |                                                          \
   ((y & 256) >> 6) |                                                          \
   ((((x) + (h) + 1) & 256) >> 7) |                                            \
   ((x) & 1))

#define SPRHDR(x, y, a, h) { SPRPOS((x),(y)), SPRCTL((x), (y), (a), (h)) }
#define SPREND() SPRHDR(0, 0, 0, 0)

#endif /* !_SPRITE_H_ */
