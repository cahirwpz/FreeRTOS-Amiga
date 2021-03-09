#ifndef _SPRITE_H_
#define _SPRITE_H_

#include <stdint.h>
#include <copper.h>

/*
 * Sprites are described here:
 * http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node00AE.html
 */

typedef struct sprdat {
  uint16_t lo, hi;
} sprdat_t;

typedef struct sprite {
  short height;
  sprdat_t *data;
  sprdat_t *attached;
} sprite_t;

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
  (((((Y) + (H) + 1) & 255) << 8) |                                            \
   (((A) & 1) << 7) |                                                          \
   (((Y) & 256) >> 6) |                                                        \
   ((((Y) + (H) + 1) & 256) >> 7) |                                            \
   ((X) & 1))

#define SPRHDR(x, y, a, h)                                                     \
  (sprdat_t){ SPRPOS((x),(y)), SPRCTL((x), (y), (a), (h)) }

#define SPREND() (sprdat_t){ 0, 0 }

extern sprdat_t _empty_spr[];

static inline void SpriteUpdatePos(sprite_t *spr, short x, short y) {
  spr->data[0] = SPRHDR(x, y, 0, spr->height);
  if (spr->attached)
    spr->attached[0] = SPRHDR(x, y, 1, spr->height);
}

static inline copins_t *CopLoadSprite(coplist_t *list, int num, sprite_t *spr) {
  return CopMove32(list, sprpt[num], spr ? spr->data : _empty_spr);
}

#endif /* !_SPRITE_H_ */
