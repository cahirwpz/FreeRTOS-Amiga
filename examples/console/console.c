#include <stdarg.h>
#include <stdio.h>
#include <bitmap.h>
#include <copper.h>
#include <font.h>
#include <palette.h>
#include <sprite.h>
#include <string.h>

#include "console.h"
#include "event.h"

#include "data/lat2-08.c"
#include "data/pointer.c"
#include "data/screen.c"

#define cons_width (screen_bm.width / 8)
#define cons_height (screen_bm.height / console_font.height)
#define cons_rowsize (screen_bm.bytesPerRow * console_font.height)

static struct {
  unsigned char *here;
  short x;
  short y;
} cursor;

/*
 * Copper configures hardware each frame (50Hz in PAL) to:
 *  - set video mode to HIRES (640x256),
 *  - display one bitplane,
 *  - set background color to black, and foreground to white,
 *  - set up mouse pointer palette,
 *  - set sprite 0 to mouse pointer graphics,
 *  - set other sprites to empty graphics,
 */
static void BuildCopperList(coplist_t *cp) {
  CopSetupScreen(cp, &screen_bm, MODE_HIRES, HP(0), VP(0));
  CopSetupBitplanes(cp, &screen_bm, NULL);
  CopLoadColor(cp, 0, 0x000);
  CopLoadColor(cp, 1, 0xfff);
  CopLoadPal(cp, &pointer_pal, 16);
  CopLoadSprite(cp, 0, &pointer_spr);
  for (int i = 1; i < 8; i++)
    CopLoadSprite(cp, i, NULL);
  CopEnd(cp);
}

void ConsoleInit(void) {
  static COPLIST(cp, 40);

  EventQueueInit();
  MouseInit(PushMouseEventFromISR, 0, 0, 319, 255);
  KeyboardInit(PushKeyEventFromISR);

  /* Set cursor to (0, 0) */
  cursor.here = screen_bm.planes[0];
  cursor.x = 0;
  cursor.y = 0;

  /* Tell copper where the copper list begins and enable copper DMA. */
  BuildCopperList(cp);
  CopListActivate(cp);

  /* Set sprite position in upper-left corner of display window. */
  SpriteUpdatePos(&pointer_spr, HP(0), VP(0));

  /* Enable bitplane and sprite fetchers' DMA. */
  EnableDMA(DMAF_RASTER | DMAF_SPRITE);
}

static inline void UpdateHere(void) {
  cursor.here = screen_bm.planes[0] +
    (short)cons_rowsize * cursor.y + cursor.x;
}

void ConsoleMovePointer(short x, short y) {
  SpriteUpdatePos(&pointer_spr, HP(x), VP(y));
}

void ConsoleSetCursor(short x, short y) {
  if (x < 0)
    cursor.x = 0;
  else if (x >= cons_width)
    cursor.x = cons_width - 1;
  else
    cursor.x = x;

  if (y < 0)
    cursor.y = 0;
  else if (y >= cons_height)
    cursor.y = cons_height - 1;
  else
    cursor.y = y;

  UpdateHere();
}

void ConsoleGetCursor(short *xp, short *yp) {
  *xp = cursor.x;
  *yp = cursor.y;
}

#pragma GCC push_options
#pragma GCC optimize ("-O3")

static void ConsoleDrawChar(int c) {
  unsigned char *src = console_font_glyphs + (c - 32) * console_font.height;
  unsigned char *dst = cursor.here++;

  for (short i = 0; i < console_font.height; i++) {
    *dst = *src++;
    dst += screen_bm.bytesPerRow;
  }
}

void ConsoleDrawCursor(void) {
  unsigned char *dst = cursor.here++;

  for (short i = 0; i < console_font.height; i++) {
    *dst = ~*dst;
    dst += screen_bm.bytesPerRow;
  }
}

#pragma GCC pop_options

static void ConsoleNextLine(void) {
  for (short x = cursor.x; x < cons_width; x++)
    ConsoleDrawChar(' ');
  cursor.x = 0;
  if (++cursor.y >= cons_height)
    cursor.y = 0;
  UpdateHere();
}

void ConsolePutChar(char c) {
  if (c < 32) {
    if (c == '\n')
      ConsoleNextLine();
  } else {
    ConsoleDrawChar(c);
    if (++cursor.x >= cons_width)
      ConsoleNextLine();
  }
}

void ConsoleWrite(const char *buf, size_t nbyte) {
  for (size_t i = 0; i < nbyte; i++)
    ConsolePutChar(*buf++);
}

void ConsolePrintf(const char *fmt, ...) {
  va_list ap;

  va_start(ap, fmt);
  kvprintf(ConsolePutChar, fmt, ap);
  va_end(ap);
}
