#ifndef _COPPER_H_
#define _COPPER_H_

#include <stdint.h>
#include <stddef.h>
#include <custom.h>

/* Copper instructions assumptions for PAL systems:
 *
 * Copper resolution is 1 color clock = 4 pixels in low resolution.
 *
 * > Vertical Position range is 0..311
 *   Keep it mind that 'vp' counter overflows at 255 !
 *
 * > Horizontal Position range is in 0..266
 *   In fact WAIT & SKIP uses 'hp' without the least significant bit !
 *
 * MOVE & SKIP take 2 color clocks
 * WAIT takes 3 color clocks (last to wake up)
 */

/* Horizontal position relative to display window. */
#ifndef HP
#define HP(x) ((x) + 0x81)
#endif

/* Vertical position relative to display window. */
#ifndef VP
#define VP(y) ((y) + 0x2c)
#endif

/* Last Horizontal Position in line one can reliably wait on. */
#define HP_LAST 0xde

/* 
 * Copper is described here:
 * http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0047.html
 */

typedef union {
  struct {
    uint8_t vp;
    uint8_t hp;
    uint8_t vpmask;
    uint8_t hpmask;
  };
  struct {
    uint16_t reg;
    uint16_t data;
  };
} copins_t;

typedef struct {
  copins_t *curr;
  copins_t *list;
} coplist_t;

#define COPLIST(NAME, SIZE)                                                    \
  __bsschip copins_t NAME##_ins[SIZE];                                         \
  coplist_t *NAME = &(coplist_t){NAME##_ins, NAME##_ins}

/* Low-level functions */
static inline copins_t *CopMoveWord(coplist_t *list, uint16_t reg,
                                    uint16_t data) {
  copins_t *ins = list->curr++;
  ins->reg = reg & 0x1fe;
  ins->data = data;
  return ins;
}

static inline copins_t *CopMoveLong(coplist_t *list, uint16_t reg, void *ptr) {
  copins_t *ins = CopMoveWord(list, reg, (intptr_t)ptr >> 16);
  CopMoveWord(list, reg + 2, (intptr_t)ptr);
  return ins;
}

#define CSREG(reg) (uint16_t)offsetof(struct Custom, reg)
#define CopMove16(list, reg, data) CopMoveWord(list, CSREG(reg), data)
#define CopMove32(list, reg, data) CopMoveLong(list, CSREG(reg), data)

static inline void CopInsSet32(copins_t *ins, void *data) {
  ins[1].data = (intptr_t)data;
  ins[0].data = (intptr_t)data >> 16;
}

static inline copins_t *CopWaitMask(coplist_t *list, uint8_t vp, uint8_t hp,
                                    uint8_t vpmask, uint8_t hpmask) {
  copins_t *ins = list->curr++;
  ins->vp = vp;
  ins->hp = hp | 1;
  ins->vpmask = 0x80 | vpmask;
  ins->hpmask = hpmask & 0xfe;
  return ins;
}

#define CopWait(list, vp, hp) CopWaitMask(list, vp, hp, 255, 255)
#define CopEnd(list) CopWaitMask(list, 255, 255, 255, 255)

static inline copins_t *CopSkipMask(coplist_t *list, uint8_t vp, uint8_t hp,
                                    uint8_t vpmask, uint8_t hpmask) {
  copins_t *ins = list->curr++;
  ins->vp = vp;
  ins->hp = hp | 1;
  ins->vpmask = 0x80 | vpmask;
  ins->hpmask = hpmask | 1;
  return ins;
}

#define CopSkip(list, vp, hp) CopSkipMask(list, vp, hp, 255, 255)

static inline void CopListActivate(coplist_t *list) {
  /* Enable copper DMA */
  EnableDMA(DMAF_COPPER);
  /* Write copper list address. */
  custom.cop1lc = (intptr_t)list->list;
}

#define MODE_LORES  0
#define MODE_HIRES  BPLCON0_HIRES
#define MODE_DUALPF BPLCON0_DBLPF
#define MODE_LACE   BPLCON0_LACE
#define MODE_HAM    BPLCON0_HOMOD

static inline void CopSetupMode(coplist_t *list, uint16_t mode, uint16_t depth) {
  CopMove16(list, bplcon0, BPLCON0_BPU(depth) | BPLCON0_COLOR | mode);
  CopMove16(list, bplcon2, BPLCON2_PF2P2 | BPLCON2_PF1P2 | BPLCON2_PF2PRI);
  CopMove16(list, bplcon3, 0);
}

static inline void CopSetupDisplayWindow(coplist_t *list, uint16_t mode,
                                         uint16_t xs, uint16_t ys,
                                         uint16_t w, uint16_t h) {
  /* vstart  $00 ..  $ff */
  /* hstart  $00 ..  $ff */
  /* vstop   $80 .. $17f */
  /* hstop  $100 .. $1ff */
  if (mode & MODE_HIRES)
    w >>= 1;
  if (mode & MODE_LACE)
    h >>= 1;

  uint8_t xe = xs + w;
  uint8_t ye = ys + h;

  CopMove16(list, diwstrt, (ys << 8) | xs);
  CopMove16(list, diwstop, (ye << 8) | xe);
}

static inline void CopSetupBitplaneFetch(coplist_t *list, uint16_t mode,
                                         uint16_t xs, uint16_t w) {
  uint8_t ddfstrt, ddfstop;

  /* DDFSTRT and DDFSTOP have resolution of 4 clocks.
   *
   * Only bits 7..2 of DDFSTRT and DDFSTOP are meaningful on OCS!
   *
   * Values to determine Display Data Fetch Start and Stop must be divisible by
   * 16 pixels, because hardware fetches bitplanes in 16-bit word units.
   * Bitplane fetcher uses 4 clocks for HiRes (1 clock = 4 pixels) and 8 clocks
   * for LoRes (1 clock = 2 pixels) to fetch enough data to display it.
   *
   * HS = Horizontal Start, W = Width (divisible by 16)
   *
   * For LoRes: DDFSTART = HS / 2 - 8.5, DDFSTOP = DDFSTRT + W / 2 - 8
   * For HiRes: DDFSTART = HS / 2 - 4.5, DDFSTOP = DDFSTRT + W / 4 - 8 */

  if (mode & MODE_HIRES) {
    xs -= 7; /* should be 9, but it does not align with sprite position */
    w >>= 2;
    ddfstrt = (xs >> 1) & ~3; /* 4 clock resolution */
  } else {
    xs -= 17;
    w >>= 1;
    ddfstrt = (xs >> 1) & ~7; /* 8 clock resolution */
  }

  ddfstop = ddfstrt + w - 8;

  /* Found in UAE source code - DDFSTRT & DDFSTOP matching for:
   * - ECS: does not require DMA or DIW enabled,
   * - OCS: requires DMA and DIW enabled. */

  CopMove16(list, ddfstrt, ddfstrt);
  CopMove16(list, ddfstop, ddfstop);
  CopMove16(list, bplcon1, ((xs & 15) << 4) | (xs & 15));
  CopMove16(list, fmode, 0);
}

#endif /* !_COPPER_H_ */
