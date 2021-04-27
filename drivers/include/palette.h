#pragma once

#include <stdint.h>

typedef struct CopList CopList_t;
typedef union CopIns CopIns_t;

typedef struct Palette {
  uint16_t count;
  uint16_t colors[]; /* 0000rrrrggggbbbb */
} Palette_t;

CopIns_t *CopLoadPal(CopList_t *list, Palette_t *pal, short start);

#define CopLoadColor(CP, NUM, RGB) CopMove16((CP), color[(NUM)], (RGB))
