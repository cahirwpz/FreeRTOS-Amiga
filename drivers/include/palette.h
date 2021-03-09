#ifndef _PALETTE_H_
#define _PALETTE_H_

#include <copper.h>

typedef struct palette {
  uint16_t count;
  uint16_t colors[]; /* 0000rrrrggggbbbb */
} palette_t;

copins_t *CopLoadPal(coplist_t *list, palette_t *pal, uint16_t start) {
  copins_t *ins = list->curr;
  short n = pal->count;
  
  if (n + start > 32)
    n = 32 - start;

  for (short i = 0; i < n; i++)
    CopMove16(list, color[i + start], pal->colors[i]);
  return ins;
}

#define CopLoadColor(CP, NUM, RGB) CopMove16((CP), color[(NUM)], (RGB))

#endif /* !_PALETTE_H_ */
