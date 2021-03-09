#ifndef _FONT_H_
#define _FONT_H_

typedef struct font {
  short height;
  unsigned char *glyphs;
  short offset[];
} font_t;

#endif
