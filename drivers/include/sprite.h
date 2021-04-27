#pragma once

#include <stdint.h>

typedef union CopIns CopIns_t;
typedef struct CopList CopList_t;

/*
 * Sprites are described here:
 * http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node00AE.html
 */

typedef struct SprDat {
  uint16_t lo, hi;
} SprDat_t;

typedef struct Sprite {
  short height;
  SprDat_t *data;
  SprDat_t *attached;
} Sprite_t;

/*
 * SPRxPOS:
 *  Bits 15-8 contain the low 8 bits of VSTART
 *  Bits 7-0 contain the high 8 bits of HSTART
 */
#define SPRPOS(X, Y) (((Y) << 8) | (((X) >> 1) & 255))

/*
 * SPRxCTL:
 *  Bits 15-8       The low eight bits of VSTOP
 *  Bit 7           (Used in attachment)
 *  Bits 6-3        Unused (make zero)
 *  Bit 2           The VSTART high bit
 *  Bit 1           The VSTOP high bit
 *  Bit 0           The HSTART low bit
 */
#define SPRCTL(X, Y, A, H)                                                     \
  (((((Y) + (H) + 1) & 255) << 8) | (((A)&1) << 7) | (((Y)&256) >> 6) |        \
   ((((Y) + (H) + 1) & 256) >> 7) | ((X)&1))

#define SPRHDR(x, y, a, h)                                                     \
  (SprDat_t) {                                                                 \
    SPRPOS((x), (y)), SPRCTL((x), (y), (a), (h))                               \
  }

#define SPREND()                                                               \
  (SprDat_t) {                                                                 \
    0, 0                                                                       \
  }

void SpriteUpdatePos(Sprite_t *spr, short x, short y);
CopIns_t *CopLoadSprite(CopList_t *list, int num, Sprite_t *spr);
