#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <bitmap.h>
#include <copper.h>
#include <font.h>
#include <palette.h>
#include <sprite.h>

#include <console.h>
#include <cpu.h>
#include <device.h>
#include <ioreq.h>
#include <libkern.h>
#include <string.h>
#include <sys/errno.h>

#include "data/lat2-08.c"
#include "data/pointer.c"

#define WIDTH 640
#define HEIGHT 256
#define DEPTH 1

#define BGCOL 0x004
#define FGCOL 0xfff

#define FONT_W 8
#define FONT_H console_font.height

#define NCOL (WIDTH / FONT_W)
#define NROW (HEIGHT / FONT_H)

typedef struct ConsoleDev {
  SemaphoreHandle_t lock;
  struct {
    uint8_t *here;
    short x, y;
  } c;

  /* Hardware dependant bits. */
  Bitmap_t bm; /* rows are stored in a non-contiguous manner */
  CopList_t cp;
  void **rowptr;     /* tells where each row is stored in `bm` bitmap */
  CopIns_t **rowins; /* update those instructions whenever `rows` changes */
  short rowsize;     /* bytes between two character rows in the bitmap */
} ConsoleDev_t;

static void ConsoleSetCursor(ConsoleDev_t *cons, short x, short y);
static void ConsoleDrawCursor(ConsoleDev_t *cons);

static int ConsoleWrite(Device_t *, IoReq_t *);

static DeviceOps_t ConsoleOps = {
  .write = ConsoleWrite,
};

/*
 * Copper configures hardware each frame (50Hz in PAL) to:
 *  - set video mode to HIRES (640x256),
 *  - display one bitplane,
 *  - set background color to black, and foreground to white,
 *  - set up mouse pointer palette,
 *  - set sprite 0 to mouse pointer graphics,
 *  - set other sprites to empty graphics,
 */
static void MakeCopList(CopList_t *cp, Bitmap_t *bm, CopIns_t **rowins,
                        void **rowptr) {
  CopListInit(cp, 80 + NROW * 3);

  CopSetupMode(cp, MODE_HIRES, DEPTH);
  CopSetupDisplayWindow(cp, MODE_HIRES, HP(0), VP(0), WIDTH, HEIGHT);
  CopSetupBitplaneFetch(cp, MODE_HIRES, HP(0), WIDTH);
  CopSetupBitplanes(cp, bm, NULL);
  CopLoadColor(cp, 0, BGCOL);
  CopLoadColor(cp, 1, FGCOL);
  CopLoadPal(cp, &pointer_pal, 16);
  CopLoadSprite(cp, 0, &pointer_spr);
  for (short i = 1; i < 8; i++)
    CopLoadSprite(cp, i, NULL);
  for (short i = 0; i < NROW; i++) {
    CopWaitSafe(cp, VP(i * FONT_H), 0);
    rowins[i] = CopMove32(cp, bplpt[0], rowptr[i]);
  }
  CopEnd(cp);

  /* Tell copper where the copper list begins and enable copper DMA. */
  CopListActivate(cp);
}

static ConsoleDev_t ConsoleDev[1];

Device_t *ConsoleInit(void) {
  ConsoleDev_t *cons = ConsoleDev;

  klog("[Console] Initializing driver!\n");

  cons->lock = xSemaphoreCreateMutex();

  /* Allocate an extra row of characters, since it will be used with copper
   * for use with smart scrolling and line insertion / deletion. */
  BitmapInit(&cons->bm, WIDTH, HEIGHT + FONT_H, DEPTH, 0);

  cons->rowsize = muls16(FONT_H, cons->bm.bytesPerRow);
  cons->rowptr = kmalloc(sizeof(void *) * (NROW + 1));
  cons->rowins = kmalloc(sizeof(CopIns_t *) * NROW);

  for (short i = 0; i < NROW + 1; i++)
    cons->rowptr[i] = cons->bm.planes[0] + muls16(i, cons->rowsize);

  MakeCopList(&cons->cp, &cons->bm, cons->rowins, cons->rowptr);
  ConsoleSetCursor(cons, 0, 0);
  ConsoleDrawCursor(cons);

  /* Set sprite position in upper-left corner of display window. */
  SpriteUpdatePos(&pointer_spr, HP(0), VP(0));

  /* Enable bitplane and sprite fetchers' DMA. */
  EnableDMA(DMAF_RASTER | DMAF_SPRITE);

  Device_t *dev;
  AddDevice("console", &ConsoleOps, &dev);
  dev->data = cons;
  return dev;
}

static inline void UpdateHere(ConsoleDev_t *cons) {
  cons->c.here = cons->rowptr[cons->c.y] + cons->c.x;
}

static inline void UpdateRows(ConsoleDev_t *cons) {
  for (short i = 0; i < NROW; i++)
    CopInsSet32(cons->rowins[i], cons->rowptr[i]);
}

void ConsoleMovePointer(short x, short y) {
  SpriteUpdatePos(&pointer_spr, HP(x), VP(y));
}

static void ConsoleSetCursor(ConsoleDev_t *cons, short x, short y) {
  if (x < 0)
    cons->c.x = 0;
  else if (x >= NCOL)
    cons->c.x = NCOL - 1;
  else
    cons->c.x = x;

  if (y < 0)
    cons->c.y = 0;
  else if (y >= NROW)
    cons->c.y = NROW - 1;
  else
    cons->c.y = y;

  UpdateHere(cons);
}

#pragma GCC push_options
#pragma GCC optimize("-O3")

static void ConsoleDrawChar(ConsoleDev_t *cons, int c) {
  uint8_t *src = console_font_glyphs + (c - 32) * FONT_H;
  uint8_t *dst = cons->c.here++;

  for (short i = 0; i < FONT_H; i++) {
    *dst = *src++;
    dst += cons->bm.bytesPerRow;
  }
}

static void ConsoleDrawCursor(ConsoleDev_t *cons) {
  uint8_t *dst = cons->c.here;

  for (short i = 0; i < FONT_H; i++) {
    *dst = ~*dst;
    dst += cons->bm.bytesPerRow;
  }
}

#pragma GCC pop_options

static void ScrollUp(ConsoleDev_t *cons) {
  void *first = cons->rowptr[0];
  memset(first, 0, cons->rowsize);
  memmove(&cons->rowptr[0], &cons->rowptr[1], NROW * sizeof(void *));
  cons->rowptr[NROW] = first;
  UpdateRows(cons);
}

static void ConsoleNextLine(ConsoleDev_t *cons) {
  cons->c.x = 0;
  if (cons->c.y < NROW - 1) {
    cons->c.y++;
  } else {
    ScrollUp(cons);
  }
  UpdateHere(cons);
}

#define ISCONTROL(c) (((c) <= 0x1f) || (((c) >= 0x7f) && ((c) <= 0x9f)))

static void ControlCode(ConsoleDev_t *cons, short c) {
  if (c == '\n')
    ConsoleNextLine(cons);
}

static int ConsoleWrite(Device_t *dev, IoReq_t *req) {
  ConsoleDev_t *cons = dev->data;

  xSemaphoreTake(cons->lock, portMAX_DELAY);

  ConsoleDrawCursor(cons);

  for (; req->left; req->left--) {
    uint8_t c = *req->wbuf++;

    if (ISCONTROL(c)) {
      ControlCode(cons, c);
    } else {
      ConsoleDrawChar(cons, c);
      if (++cons->c.x >= NCOL)
        ConsoleNextLine(cons);
    }
  }

  ConsoleDrawCursor(cons);

  xSemaphoreGive(cons->lock);

  return 0;
}
