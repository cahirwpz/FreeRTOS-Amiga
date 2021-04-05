#include <cpu.h>
#include <copper.h>
#include <libkern.h>

void CopListInit(CopList_t *list, uint16_t length) {
  list->curr = list->list = kmalloc_chip(muls16(length, sizeof(CopIns_t)));
}

void CopListKill(CopList_t *list) {
  kfree(list->list);
}

void CopListActivate(CopList_t *list) {
  /* Enable copper DMA */
  EnableDMA(DMAF_COPPER);
  /* Write copper list address. */
  custom.cop1lc = (intptr_t)list->list;
}

void CopSetupMode(CopList_t *list, uint16_t mode, uint16_t depth) {
  CopMove16(list, bplcon0, BPLCON0_BPU(depth) | BPLCON0_COLOR | mode);
  CopMove16(list, bplcon2, BPLCON2_PF2P2 | BPLCON2_PF1P2 | BPLCON2_PF2PRI);
  CopMove16(list, bplcon3, 0);
}

void CopSetupDisplayWindow(CopList_t *list, uint16_t mode, uint16_t xs,
                           uint16_t ys, uint16_t w, uint16_t h) {
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

void CopSetupBitplaneFetch(CopList_t *list, uint16_t mode, uint16_t xs,
                           uint16_t w) {
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
