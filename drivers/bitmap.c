#include <bitmap.h>
#include <copper.h>
#include <cpu.h>
#include <string.h>
#include <libkern.h>

void CopSetupScreen(CopList_t *list, const Bitmap_t *bm, uint16_t mode,
                    uint16_t xs, uint16_t ys) {
  CopSetupMode(list, mode, bm->depth);
  CopSetupDisplayWindow(list, mode, xs, ys, bm->width, bm->height);
  CopSetupBitplaneFetch(list, mode, xs, bm->width);
}

void CopSetupBitplanes(CopList_t *list, const Bitmap_t *bm, CopIns_t **bplptr) {
  for (short i = 0; i < bm->depth; i++) {
    CopIns_t *ins = CopMove32(list, bplpt[i], bm->planes[i]);
    if (bplptr)
      *bplptr++ = ins;
  }

  short modulo = 0;

  if (bm->flags & BM_INTERLEAVED)
    modulo = muls16(bm->bytesPerRow, bm->depth - 1);

  CopMove16(list, bpl1mod, modulo);
  CopMove16(list, bpl2mod, modulo);
}

void BitmapInit(Bitmap_t *bm, uint16_t width, uint16_t height, uint16_t depth,
                BmFlags_t flags) {
  uint16_t bytesPerRow = ((width + 15) & ~15) / 8;
  int bplSize = muls16(bytesPerRow, height);
  long size = muls16(bplSize, depth);
  void *planes = kmalloc_chip(size);
  memset(planes, 0, size);

  bm->width = width;
  bm->height = height;
  bm->depth = depth;
  bm->bytesPerRow = bytesPerRow;
  bm->flags = flags;

  int modulo = (flags & BM_INTERLEAVED) ? bytesPerRow : bplSize;

  for (short i = 0; i < (short)depth; i++, planes += modulo)
    bm->planes[i] = planes;
}
