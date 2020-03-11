#include <blitter.h>

static uint16_t LineMode[4][2] = {
  {BC0F_LINE_OR, LINEMODE},
  {BC0F_LINE_EOR, LINEMODE},
  {BC0F_LINE_OR, LINEMODE | ONEDOT},
  {BC0F_LINE_EOR, LINEMODE | ONEDOT}
};

/*
 * Minterm is either:
 * - OR: (ABC | ABNC | NABC | NANBC)
 * - XOR: (ABNC | NABC | NANBC)
 */

/*
 *  \   |   /
 *   \3 | 1/
 *  7 \ | / 6
 *     \|/
 *  ----X----
 *     /|\
 *  5 / | \ 4
 *   /2 | 0\
 *  /   |   \
 *
 * OCT | SUD SUL AUL
 * ----+------------
 *   3 |   1   1   1
 *   0 |   1   1   0
 *   4 |   1   0   1
 *   7 |   1   0   0
 *   2 |   0   1   1
 *   5 |   0   1   0
 *   1 |   0   0   1
 *   6 |   0   0   0
 */

#define xs bl->xs
#define ys bl->ys
#define xe bl->xe
#define ye bl->ye
#define mode bl->mode
#define pattern bl->pattern
#define stride bl->stride

#define _start bl->_start
#define _bltapt bl->_bltapt
#define _bltsize bl->_bltsize

static __bsschip uint16_t scratch[1];

void BltLineSetup(bltline_t *bl) {
  uint16_t bltcon0 = LineMode[mode][0];
  uint16_t bltcon1 = LineMode[mode][1];

  short x1 = xs;
  short y1 = ys;
  short x2 = xe;
  short y2 = ye;

  /* Always draw the line downwards. */
  if (y1 > y2) {
    swap(x1, x2);
    swap(y1, y2);
  }

  bltcon0 |= ASHIFT(x1);

  /* Word containing the first pixel of the line. */
  _start = stride * y1 + ((x1 >> 3) & ~1);

  short dx = x2 - x1;
  short dy = y2 - y1;

  if (dx < 0) {
    dx = -dx;
    if (dx >= dy) {
      bltcon1 |= AUL | SUD;
    } else {
      bltcon1 |= SUL;
      swap(dx, dy);
    }
  } else {
    if (dx >= dy) {
      bltcon1 |= SUD;
    } else {
      swap(dx, dy);
    }
  }

  short derr = dy + dy - dx;
  if (derr < 0)
    bltcon1 |= SIGNFLAG;

  _bltapt = derr;
  _bltsize = (dx << 6) + 66;

  short bltamod = derr - dx;
  short bltbmod = dy + dy;

  WaitBlitter();

  custom.bltafwm = -1;
  custom.bltalwm = -1;
  custom.bltadat = 0x8000;
  custom.bltbdat = pattern;
  custom.bltcmod = stride;
  custom.bltdmod = stride;
  custom.bltcon0 = bltcon0;
  custom.bltcon1 = bltcon1;
  custom.bltamod = bltamod;
  custom.bltbmod = bltbmod;
}

void BltLine(bltline_t *bl, void *dstbpl) {
  void *data = dstbpl + _start;
  void *bltdpt = (mode & LINE_ONEDOT) ? scratch : data;

  WaitBlitter();

  custom.bltapt = (void *)_bltapt;
  custom.bltcpt = data;
  custom.bltdpt = bltdpt;
  custom.bltsize = _bltsize;
}
