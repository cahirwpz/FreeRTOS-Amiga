#include <stdarg.h>
#include <stdio.h>
#include <bitmap.h>
#include <font.h>

#include "console.h"

typedef struct console {
  bitmap_t *bitmap;
  font_t *font;
  short width;
  short height;
  struct {
    short x;
    short y;
  } cursor;
} console_t;

static console_t cons;

void ConsoleInit(bitmap_t *bm, font_t *font) {
  cons.bitmap = bm;
  cons.font = font;
  cons.width = bm->width / 8;
  cons.height = bm->height / font->height;
  cons.cursor.x = 0;
  cons.cursor.y = 0;
}

void ConsoleSetCursor(short x, short y) {
  if (x < 0)
    cons.cursor.x = 0;
  else if (x >= cons.width)
    cons.cursor.x = cons.width - 1;
  else
    cons.cursor.x = x;

  if (y < 0)
    cons.cursor.y = 0;
  else if (y >= cons.height)
    cons.cursor.y = cons.height - 1;
  else
    cons.cursor.y = y;
}

static void ConsoleDrawChar(short x, short y, uint8_t c) {
  unsigned char *src = cons.font->glyphs;
  unsigned char *dst = cons.bitmap->planes[0];
  short dwidth = cons.bitmap->bytesPerRow;
  short h = cons.font->height - 1;

  src += cons.font->offset[c];
  dst += cons.bitmap->width * y + x;

  do {
    *dst = *src++; dst += dwidth;
  } while (--h != -1);
}

void ConsoleDrawCursor(void) {
  unsigned char *dst = cons.bitmap->planes[0];
  short dwidth = cons.bitmap->bytesPerRow;
  short h = cons.font->height - 1;

  dst += cons.bitmap->width * cons.cursor.y + cons.cursor.x;

  do {
    *dst = ~*dst; dst += dwidth;
  } while (--h != -1);
}

static void ConsoleNextLine(void) {
  for (short x = cons.cursor.x; x < cons.width; x++)
    ConsoleDrawChar(x, cons.cursor.y, ' ');
  cons.cursor.x = 0;
  if (++cons.cursor.y >= cons.height)
    cons.cursor.y = 0;
}

static void ConsoleNextChar(void) {
  if (++cons.cursor.x >= cons.width)
    ConsoleNextLine();
}

void ConsolePutChar(char c) {
  switch (c) {
    case '\r':
      break;
    case '\n':
      ConsoleNextLine();
      break;
    default:
      if (c < 32)
        return;
      ConsoleDrawChar(cons.cursor.x, cons.cursor.y, c);
      ConsoleNextChar();
      break;
  }
}


void ConsolePrintf(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  kvprintf(ConsolePutChar, fmt, ap);
  va_end(ap);
}
