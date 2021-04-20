#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <bitmap.h>
#include <copper.h>
#include <font.h>
#include <palette.h>
#include <sprite.h>

#include <display.h>
#include <driver.h>
#include <file.h>
#include <devfile.h>
#include <ioreq.h>
#include <memory.h>
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
#define FONT_H display_font.height

#define NCOL (WIDTH / FONT_W)
#define NROW (HEIGHT / FONT_H)

typedef struct DisplayDev {
  SemaphoreHandle_t lock;
  DevFile_t *file;
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
} DisplayDev_t;

static void DisplaySetCursor(DisplayDev_t *disp, short x, short y);
static void DisplayDrawCursor(DisplayDev_t *disp);

static int DisplayOpen(DevFile_t *, FileFlags_t);
static int DisplayClose(DevFile_t *, FileFlags_t);
static int DisplayWrite(DevFile_t *, IoReq_t *);
static int DisplayIoctl(DevFile_t *, u_long, void *, FileFlags_t);

static DevFileOps_t DisplayOps = {
  .type = DT_OTHER,
  .open = DisplayOpen,
  .close = DisplayClose,
  .read = NullDevRead,
  .write = DisplayWrite,
  .strategy = NullDevStrategy,
  .ioctl = DisplayIoctl,
  .event = NullDevEvent,
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

static int DisplayOpen(DevFile_t *dev, FileFlags_t flags) {
  if (flags & F_READ)
    return EACCES;

  if (!dev->usecnt) {
    DisplayDev_t *disp = dev->data;

    /* Allocate an extra row of characters, since it will be used with copper
     * for use with smart scrolling and line insertion / deletion. */
    BitmapInit(&disp->bm, WIDTH, HEIGHT + FONT_H, DEPTH, 0);

    disp->rowsize = muls16(FONT_H, disp->bm.bytesPerRow);
    disp->rowptr = MemAlloc(sizeof(void *) * (NROW + 1), 0);
    disp->rowins = MemAlloc(sizeof(CopIns_t *) * NROW, 0);

    for (short i = 0; i < NROW + 1; i++)
      disp->rowptr[i] = disp->bm.planes[0] + muls16(i, disp->rowsize);

    MakeCopList(&disp->cp, &disp->bm, disp->rowins, disp->rowptr);
    DisplaySetCursor(disp, 0, 0);
    DisplayDrawCursor(disp);

    /* Set sprite position in upper-left corner of display window. */
    SpriteUpdatePos(&pointer_spr, HP(0), VP(0));

    /* Enable bitplane and sprite fetchers' DMA. */
    EnableDMA(DMAF_RASTER | DMAF_SPRITE);
  }

  return 0;
}

static int DisplayClose(DevFile_t *dev, FileFlags_t flags __unused) {
  if (!dev->usecnt) {
    DisplayDev_t *disp = dev->data;

    DisableDMA(DMAF_RASTER | DMAF_SPRITE);
    CopListKill(&disp->cp);
    MemFree(disp->rowptr);
    MemFree(disp->rowins);
    BitmapKill(&disp->bm);
  }

  return 0;
}

static inline void UpdateHere(DisplayDev_t *disp) {
  disp->c.here = disp->rowptr[disp->c.y] + disp->c.x;
}

static inline void UpdateRows(DisplayDev_t *disp) {
  for (short i = 0; i < NROW; i++)
    CopInsSet32(disp->rowins[i], disp->rowptr[i]);
}

static void DisplaySetCursor(DisplayDev_t *disp, short x, short y) {
  if (x < 0)
    disp->c.x = 0;
  else if (x >= NCOL)
    disp->c.x = NCOL - 1;
  else
    disp->c.x = x;

  if (y < 0)
    disp->c.y = 0;
  else if (y >= NROW)
    disp->c.y = NROW - 1;
  else
    disp->c.y = y;

  UpdateHere(disp);
}

#pragma GCC push_options
#pragma GCC optimize("-O3")

static void DisplayDrawChar(DisplayDev_t *disp, int c) {
  uint8_t *src = display_font_glyphs + (c - 32) * FONT_H;
  uint8_t *dst = disp->c.here++;

  for (short i = 0; i < FONT_H; i++) {
    *dst = *src++;
    dst += disp->bm.bytesPerRow;
  }
}

static void DisplayDrawCursor(DisplayDev_t *disp) {
  uint8_t *dst = disp->c.here;

  for (short i = 0; i < FONT_H; i++) {
    *dst = ~*dst;
    dst += disp->bm.bytesPerRow;
  }
}

#pragma GCC pop_options

static void ScrollUp(DisplayDev_t *disp) {
  void *first = disp->rowptr[0];
  memset(first, 0, disp->rowsize);
  memmove(&disp->rowptr[0], &disp->rowptr[1], NROW * sizeof(void *));
  disp->rowptr[NROW] = first;
  UpdateRows(disp);
}

static void DisplayNextLine(DisplayDev_t *disp) {
  disp->c.x = 0;
  if (disp->c.y < NROW - 1) {
    disp->c.y++;
  } else {
    ScrollUp(disp);
  }
  UpdateHere(disp);
}

#define ISCONTROL(c) (((c) <= 0x1f) || (((c) >= 0x7f) && ((c) <= 0x9f)))

static void ControlCode(DisplayDev_t *disp, short c) {
  if (c == '\n')
    DisplayNextLine(disp);
}

static int DisplayWrite(DevFile_t *dev, IoReq_t *req) {
  DisplayDev_t *disp = dev->data;

  xSemaphoreTake(disp->lock, portMAX_DELAY);

  DisplayDrawCursor(disp);

  for (; req->left; req->left--) {
    uint8_t c = *req->wbuf++;

    if (ISCONTROL(c)) {
      ControlCode(disp, c);
    } else {
      DisplayDrawChar(disp, c);
      if (++disp->c.x >= NCOL)
        DisplayNextLine(disp);
    }
  }

  DisplayDrawCursor(disp);

  xSemaphoreGive(disp->lock);

  return 0;
}

static int DisplayIoctl(DevFile_t *dev __unused, u_long cmd, void *data,
                        FileFlags_t flags __unused) {
  if (cmd == DIOCSETMS) {
    MousePos_t *m = (MousePos_t *)data;
    SpriteUpdatePos(&pointer_spr, HP(m->x), VP(m->y));
    return 0;
  }

  return EINVAL;
}

static int DisplayAttach(Driver_t *drv) {
  DisplayDev_t *disp = drv->state;

  disp->lock = xSemaphoreCreateMutex();

  int error;
  if ((error = AddDevFile("display", &DisplayOps, &disp->file)))
    return error;

  disp->file->data = (void *)disp;
  return error;
}

Driver_t Display = {
  .name = "display",
  .attach = DisplayAttach,
  .size = sizeof(DisplayDev_t),
};
