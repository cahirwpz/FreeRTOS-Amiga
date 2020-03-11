#include <blitter.h>

static uint16_t FirstWordMask[16] = {
  0xFFFF, 0x7FFF, 0x3FFF, 0x1FFF, 0x0FFF, 0x07FF, 0x03FF, 0x01FF,
  0x00FF, 0x007F, 0x003F, 0x001F, 0x000F, 0x0007, 0x0003, 0x0001
};

static uint16_t LastWordMask[16] = {
  0xFFFF, 0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00,
  0xFF00, 0xFF80, 0xFFC0, 0xFFE0, 0xFFF0, 0xFFF8, 0xFFFC, 0xFFFE
};

#define srcstride (bc->src.bm->bytesPerRow)
#define mask (bc->src.bm->flags & BM_HASMASK)
#define sx (bc->src.x)
#define sy (bc->src.y)
#define sw (bc->src.w)
#define sh (bc->src.h)

#define dststride (bc->dst.bm->bytesPerRow)
#define dx (bc->dst.x)
#define dy (bc->dst.y)

#define _srcstart (bc->_srcstart)
#define _dststart (bc->_dststart)
#define _bltsize (bc->_bltsize)
#define _fast (bc->_fast)

#define START(x) (((x) & ~15) >> 3)
#define ALIGN(x) START((x) + 15)

/* This routine assumes following conditions:
 *  - there's always enough space in `dst` to copy area from `src`
 *  - `sx` is aligned to word boundary 
 */
void BltCopySetup(bltcopy_t *bc) {
  uint16_t xo = dx & 15;
  uint16_t width = xo + sw;
  uint16_t wo = width & 15;
  uint16_t bytesPerRow = ALIGN(width);
  uint16_t srcmod = srcstride - bytesPerRow;
  uint16_t dstmod = dststride - bytesPerRow;
  uint16_t bltafwm = FirstWordMask[xo];
  uint16_t bltalwm = LastWordMask[wo];
  uint16_t bltshift = ASHIFT(xo);

  _srcstart = START(sx) + (short)sy * (short)srcstride;
  _dststart = START(dx) + (short)dy * (short)dststride;
  _bltsize = (sh << 6) | (bytesPerRow >> 1);
  _fast = (xo == 0) && (wo == 0);

  WaitBlitter();

  if (mask) {
    custom.bltcon0 = 
      bltshift | (SRCA | SRCB | SRCC | DEST) | (ABC | NABC | ABNC | NANBC);
    custom.bltafwm = -1;
    custom.bltalwm = bltshift ? 0 : -1;
  } else {
    if (_fast) {
      custom.bltcon0 = (SRCB | DEST) | (ABC | ABNC | NABC | NABNC);
    } else {
      custom.bltcon0 = (SRCB | SRCC | DEST) | (ABC | ABNC | NABC | NANBC);
    }
    custom.bltadat = -1;
    custom.bltafwm = bltafwm;
    custom.bltalwm = bltalwm;
  }

  custom.bltcon1 = bltshift;
  custom.bltamod = srcmod;
  custom.bltbmod = srcmod;
  custom.bltcmod = dstmod;
  custom.bltdmod = dstmod;
}

void BltCopy(bltcopy_t *bc, void *dstbpl, void *srcbpl, void *mskbpl) {
  void *mskbpt = mskbpl + _srcstart;
  void *srcbpt = srcbpl + _srcstart;
  void *dstbpt = dstbpl + _dststart;
  uint16_t bltsize = _bltsize;

  WaitBlitter();

  custom.bltapt = mskbpt;
  custom.bltbpt = srcbpt;
  custom.bltcpt = dstbpt;
  custom.bltdpt = dstbpt;
  custom.bltsize = bltsize;
}
