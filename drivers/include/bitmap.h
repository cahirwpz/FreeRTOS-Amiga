#pragma once

#include <sys/cdefs.h>

typedef union CopIns CopIns_t;
typedef struct CopList CopList_t;

#define MAXDEPTH 8

typedef enum BmFlags {
  BM_NORMAL = 0,
  BM_INTERLEAVED = 1,
} __packed BmFlags_t;

typedef struct Bitmap {
  int16_t width;
  int16_t height;
  int8_t depth;
  BmFlags_t flags;
  int16_t bytesPerRow;
  void *mask;
  void *planes[MAXDEPTH];
} Bitmap_t;

void BitmapInit(Bitmap_t *bm, uint16_t width, uint16_t height, uint16_t depth,
                BmFlags_t flags);

void CopSetupScreen(CopList_t *list, const Bitmap_t *bm, uint16_t mode,
                    uint16_t xs, uint16_t ys);
void CopSetupBitplanes(CopList_t *list, const Bitmap_t *bm, CopIns_t **bplptr);
