#ifndef _BITMAP_H_
#define _BITMAP_H_

#include <copper.h>

#define MAXDEPTH 8

#define BM_INTERLEAVED 1
#define BM_HASMASK 2

typedef struct bitmap {
  int16_t width;
  int16_t height;
  int8_t depth;
  int8_t flags;
  int16_t bytesPerRow;
  void *mask;
  void *planes[MAXDEPTH];
} bitmap_t;

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
