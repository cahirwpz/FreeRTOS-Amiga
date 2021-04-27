#include <copper.h>
#include <palette.h>

CopIns_t *CopLoadPal(CopList_t *list, Palette_t *pal, short start) {
  CopIns_t *ins = list->curr;
  short n = pal->count;

  if (n + start > 32)
    n = 32 - start;

  for (short i = 0; i < n; i++)
    CopMove16(list, color[i + start], pal->colors[i]);
  return ins;
}
