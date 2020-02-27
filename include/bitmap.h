#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <copper.h>

#define MAXDEPTH 8

#define BM_INTERLEAVED 1

typedef struct bitmap {
  uint16_t width;
  uint16_t height;
  uint8_t depth;
  uint8_t flags;
  uint16_t bytesPerRow;
  uint16_t bplSize;
  void *planes[MAXDEPTH];
} bitmap_t;

#define BITMAP(w, h, d)                                                        \
  {.width = (w), .height = (h), .depth = (d),                                  \
   .bytesPerRow = (((w) + 15) & ~15) / 8,                                      \
   .bplSize = (((w) + 15) & ~15) / 8 * (h)}

static inline void CopSetupBitplanes(coplist_t *list, copins_t **bplptr,
                                     bitmap_t *bm, uint16_t depth)
{
  for (int i = 0; i < depth; i++) {
    copins_t *ins = CopMove32(list, bplpt[i], bm->planes[i]);
    if (bplptr)
      *bplptr++ = ins;
  }

  short modulo = 0;

  if (bm->flags & BM_INTERLEAVED)
    modulo = (short)bm->bytesPerRow * (short)(depth - 1);

  CopMove16(list, bpl1mod, modulo);
  CopMove16(list, bpl2mod, modulo);
}

#endif /* !_BITMAP_H_ */
