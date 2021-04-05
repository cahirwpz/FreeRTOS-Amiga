#pragma once

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
#define HP(x) ((uint8_t)(x) + 0x81)
#endif

/* Vertical position relative to display window. */
#ifndef VP
#define VP(y) ((uint8_t)(y) + 0x2c)
#endif

/* Last Horizontal Position in line one can reliably wait on. */
#define HP_LAST 0xde

/*
 * Copper is described here:
 * http://amigadev.elowar.com/read/ADCD_2.1/Hardware_Manual_guide/node0047.html
 */

typedef union CopIns {
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
} CopIns_t;

typedef struct CopList {
  CopIns_t *curr;
  CopIns_t *list;
  uint8_t overflow; /* -1 if Vertical Position counter overflowed */
} CopList_t;

#define COPLIST(NAME, SIZE)                                                    \
  __bsschip CopIns_t NAME##_ins[SIZE];                                         \
  CopList_t *NAME = &(CopList_t) {                                             \
    NAME##_ins, NAME##_ins, 0                                                  \
  }

/* Low-level functions */
static inline CopIns_t *CopMoveWord(CopList_t *list, uint16_t reg,
                                    uint16_t data) {
  CopIns_t *ins = list->curr++;
  ins->reg = reg & 0x1fe;
  ins->data = data;
  return ins;
}

static inline CopIns_t *CopMoveLong(CopList_t *list, uint16_t reg, void *ptr) {
  CopIns_t *ins = CopMoveWord(list, reg, (intptr_t)ptr >> 16);
  CopMoveWord(list, reg + 2, (intptr_t)ptr);
  return ins;
}

#define CSREG(reg) (uint16_t) offsetof(struct Custom, reg)
#define CopMove16(list, reg, data) CopMoveWord(list, CSREG(reg), data)
#define CopMove32(list, reg, data) CopMoveLong(list, CSREG(reg), data)

static inline void CopInsSet32(CopIns_t *ins, void *data) {
  ins[1].data = (intptr_t)data;
  ins[0].data = (intptr_t)data >> 16;
}

static inline CopIns_t *CopWaitMask(CopList_t *list, uint8_t vp, uint8_t hp,
                                    uint8_t vpmask, uint8_t hpmask) {
  CopIns_t *ins = list->curr++;
  ins->vp = vp;
  ins->hp = hp | 1;
  ins->vpmask = 0x80 | vpmask;
  ins->hpmask = hpmask & 0xfe;
  return ins;
}

#define CopWait(list, vp, hp) CopWaitMask(list, vp, hp, 255, 255)
#define CopEnd(list) CopWaitMask(list, 255, 255, 255, 255)

/* Handles Copper Vertical Position counter overflow, by inserting CopWaitEOL
 * at first WAIT instruction with VP >= 256. */
static inline CopIns_t *CopWaitSafe(CopList_t *list, short vp, short hp) {
  if (vp > 255 && !list->overflow) {
    list->overflow = -1;
    /* Wait for last waitable position to control when overflow occurs. */
    CopWait(list, 0xff, 0xdf);
  }
  return CopWait(list, vp, hp);
}

static inline CopIns_t *CopSkipMask(CopList_t *list, uint8_t vp, uint8_t hp,
                                    uint8_t vpmask, uint8_t hpmask) {
  CopIns_t *ins = list->curr++;
  ins->vp = vp;
  ins->hp = hp | 1;
  ins->vpmask = 0x80 | vpmask;
  ins->hpmask = hpmask | 1;
  return ins;
}

#define CopSkip(list, vp, hp) CopSkipMask(list, vp, hp, 255, 255)

#define MODE_LORES 0
#define MODE_HIRES BPLCON0_HIRES
#define MODE_DUALPF BPLCON0_DBLPF
#define MODE_LACE BPLCON0_LACE
#define MODE_HAM BPLCON0_HOMOD

void CopListInit(CopList_t *list, uint16_t length);
void CopListKill(CopList_t *list);
void CopListActivate(CopList_t *list);
void CopSetupMode(CopList_t *list, uint16_t mode, uint16_t depth);
void CopSetupDisplayWindow(CopList_t *list, uint16_t mode, uint16_t xs,
                           uint16_t ys, uint16_t w, uint16_t h);
void CopSetupBitplaneFetch(CopList_t *list, uint16_t mode, uint16_t xs,
                           uint16_t w);
