#pragma once

typedef struct font {
  short height;
  unsigned char *glyphs;
  short offset[];
} font_t;
