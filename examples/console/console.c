#include <FreeRTOS/FreeRTOS.h>
#include <stdio.h>
#include <bitmap.h>
#include <font.h>
#include <file.h>

#include "console.h"

typedef struct Console {
  File_t file;
  bitmap_t *bitmap;
  font_t *font;
  short width;
  short height;
  struct {
    short x;
    short y;
  } cursor;
} Console_t;

static int ConsoleWrite(Console_t *cons, const char *buf, size_t nbyte);

static FileOps_t ConsOps = {
  .write = (FileWrite_t)ConsoleWrite,
};

File_t *ConsoleOpen(bitmap_t *bm, font_t *font) {
  Console_t *cons = pvPortMalloc(sizeof(Console_t));
  cons->file.ops = &ConsOps;
  cons->file.usecount = 1;
  cons->bitmap = bm;
  cons->font = font;
  cons->width = bm->width / 8;
  cons->height = bm->height / font->height;
  cons->cursor.x = 0;
  cons->cursor.y = 0;
  return &cons->file;
}

static __unused void ConsoleSetCursor(Console_t *cons, short x, short y) {
  if (x < 0)
    cons->cursor.x = 0;
  else if (x >= cons->width)
    cons->cursor.x = cons->width - 1;
  else
    cons->cursor.x = x;

  if (y < 0)
    cons->cursor.y = 0;
  else if (y >= cons->height)
    cons->cursor.y = cons->height - 1;
  else
    cons->cursor.y = y;
}

static void ConsoleDrawChar(Console_t *cons, short x, short y, uint8_t c) {
  unsigned char *src = cons->font->glyphs;
  unsigned char *dst = cons->bitmap->planes[0];
  short dwidth = cons->bitmap->bytesPerRow;
  short h = cons->font->height - 1;

  src += cons->font->offset[c];
  dst += cons->bitmap->width * y + x;

  do {
    *dst = *src++; dst += dwidth;
  } while (--h != -1);
}

static __unused void ConsoleDrawCursor(Console_t *cons) {
  unsigned char *dst = cons->bitmap->planes[0];
  short dwidth = cons->bitmap->bytesPerRow;
  short h = cons->bitmap->height - 1;

  dst += cons->bitmap->width * cons->cursor.y + cons->cursor.x;

  do {
    *dst = ~*dst; dst += dwidth;
  } while (--h != -1);
}

static void ConsoleNextLine(Console_t *cons) {
  for (short x = cons->cursor.x; x < cons->width; x++)
    ConsoleDrawChar(cons, x, cons->cursor.y, ' ');
  cons->cursor.x = 0;
  if (++cons->cursor.y >= cons->height)
    cons->cursor.y = 0;
}

static void ConsoleNextChar(Console_t *cons) {
  if (++cons->cursor.x >= cons->width)
    ConsoleNextLine(cons);
}

static int ConsoleWrite(Console_t *cons, const char *buf, size_t nbyte) {
  for (size_t i = 0; i < nbyte; i++) {
    char c = *buf++;
    switch (c) {
      case '\r':
        break;
      case '\n':
        ConsoleNextLine(cons);
        break;
      default:
        if (c < 32)
          continue;
        ConsoleDrawChar(cons, cons->cursor.x, cons->cursor.y, c);
        ConsoleNextChar(cons);
        break;
    }
  }
  return nbyte;
}
