#ifndef _BLITTER_H_
#define _BLITTER_H_

#include <custom.h>

/* definitions for blitter control register 0 */
#define ABC BIT(7)
#define ABNC BIT(6)
#define ANBC BIT(5)
#define ANBNC BIT(4)
#define NABC BIT(3)
#define NABNC BIT(2)
#define NANBC BIT(1)
#define NANBNC BIT(0)

#define DEST BIT(8)
#define SRCC BIT(9)
#define SRCB BIT(10)
#define SRCA BIT(11)

#define ASHIFT(x) (((x) & 15) << 12)

/* definitions for blitter control register 1 */
#define LINEMODE BIT(0)

#define BSHIFT(x) (((x) & 15) << 12)

/* bltcon1 in normal mode */
#define OVFLAG BIT(5)
#define FILL_XOR BIT(4)
#define FILL_OR BIT(3)
#define FILL_CARRYIN BIT(2)
#define BLITREVERSE BIT(1)

/* bltcon1 in line mode */
#define SIGNFLAG BIT(6)
#define SUD BIT(4)
#define SUL BIT(3)
#define AUL BIT(2)
#define ONEDOT BIT(1)

/* some commonly used operations */
#define A_AND_B (ABC | ABNC)
#define A_AND_NOT_B (ANBC | ANBNC)
#define A_OR_B (ABC | ANBC | NABC | ABNC | ANBNC | NABNC)
#define A_OR_C (ABC | NABC | ABNC | ANBC | NANBC | ANBNC)
#define A_TO_D (ABC | ANBC | ABNC | ANBNC)
#define A_XOR_B (ANBC | NABC | ANBNC | NABNC)
#define A_XOR_C (NABC | ABNC | NANBC | ANBNC)
#define C_TO_D (ABC | NABC | ANBC | NANBC)

#define HALF_ADDER ((SRCA | SRCB | DEST) | A_XOR_B)
#define HALF_ADDER_CARRY ((SRCA | SRCB | DEST) | A_AND_B)
#define FULL_ADDER ((SRCA | SRCB | SRCC | DEST) | (NANBC | NABNC | ANBNC | ABC))
#define FULL_ADDER_CARRY                                                       \
  ((SRCA | SRCB | SRCC | DEST) | (NABC | ANBC | ABNC | ABC))

#define HALF_SUB ((SRCA | SRCB | DEST) | A_XOR_B)
#define HALF_SUB_BORROW ((SRCA | SRCB | DEST) | (NABC | NABNC))

#define BC0F_LINE_OR ((ABC | ABNC | NABC | NANBC) | (SRCA | SRCC | DEST))
#define BC0F_LINE_EOR ((ABNC | NABC | NANBC) | (SRCA | SRCC | DEST))

/* Common blitter macros. */
static inline bool BlitterBusy(void) {
  return custom.dmaconr & DMAF_BLTDONE;
}

static inline void WaitBlitter(void) {
  while (BlitterBusy());
}

/* Blitter copying state & routines. */
typedef struct {
  /* public fields */
  struct {
    short x, y;
    short stride; /* destination bitplane row size (in bytes) */
  } dst;
  struct {
    short x, y;   /* x must be 16 pixels aligned */
    short w, h;   /* w must be 16 pixels aligned */
    short stride; /* source bitplane row size (in bytes) */
    bool mask;    /* copy only pixels marked as lit in the mask */
  } src;

  /* private fields */
  intptr_t _srcstart;
  intptr_t _dststart;
  uint16_t _bltsize;
  bool _fast;
} bltcopy_t;

void BltCopySetup(bltcopy_t *bc);
void BltCopy(bltcopy_t *bc, void *dstbpl, void *srcbpl, void *mskbpl);

#define BltCopySetSrc(BC, BM, X, Y, W, H)                                      \
  {                                                                            \
    (BC)->src.x = (X);                                                         \
    (BC)->src.y = (Y);                                                         \
    (BC)->src.w = (W) < 0 ? (BM)->width : (W);                                 \
    (BC)->src.h = (H) < 0 ? (BM)->height : (H);                                \
    (BC)->src.stride = (BM)->bytesPerRow;                                      \
    (BC)->src.mask = (BM)->flags & BM_HASMASK;                                 \
  }

#define BltCopySetDst(BC, BM, X, Y)                                            \
  {                                                                            \
    (BC)->dst.x = (X);                                                         \
    (BC)->dst.y = (Y);                                                         \
    (BC)->dst.stride = (BM)->bytesPerRow;                                      \
  }

/* Line drawing modes. */
typedef enum __packed {
  LINE_OR = 0,
  LINE_SOLID = 0,
  LINE_EOR = 1,
  LINE_ONEDOT = 2
} linemode_t;

/* Blitter line drawing state & routines. */
typedef struct {
  /* public fields */
  short xs, ys;
  short xe, ye;
  short stride;     /* Distance between two lines of bitplane (in bytes). */
  linemode_t mode;
  uint16_t pattern; /* Line texture pattern. */

  /* private fields */
  intptr_t _start;
  intptr_t _bltapt;
  uint16_t _bltsize;
} bltline_t;

void BltLineSetup(bltline_t *bl);
void BltLine(bltline_t *bl, void *dstbpl);

#endif
