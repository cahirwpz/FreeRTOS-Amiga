#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdalign.h>

#define abs(x)                                                                 \
  ({                                                                           \
    typeof(x) _x = (x);                                                        \
    (_x < 0) ? -_x : _x;                                                       \
  })

#define min(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    _a < _b ? _a : _b;                                                         \
  })

#define max(a, b)                                                              \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(b) _b = (b);                                                        \
    _a > _b ? _a : _b;                                                         \
  })

#define swap(a, b)                                                             \
  ({                                                                           \
    typeof(a) _a = (a);                                                        \
    typeof(a) _b = (b);                                                        \
    (a) = _b;                                                                  \
    (b) = _a;                                                                  \
  })

/* Use on 68000 & 68010 (which do not have full 32-bit multiplication)
 * when 16-bit by 16-bit with 32-bit result multiplication will do. */
static inline int32_t muls16(int16_t a, int16_t b) {
  int32_t r;
  asm("muls %2,%0" : "=d"(r) : "0"(a), "dmi"(b));
  return r;
}

typedef struct div16 {
  int16_t rem;  /* remainder */
  int16_t quot; /* quotient */
} div16_t;

/* Use on 68000 & 68010 (which do not have full 32-bit division)
 * when 32-bit by 16-bit with 16-bit result division will do. */
static inline div16_t divs16(int32_t a, int16_t b) {
  div16_t r;
  asm("divs %2,%0" : "=d"(r) : "0"(a), "dmi"(b));
  return r;
}

#define BIT(x) (1L << (x))

#define REGB(addr) (*(volatile uint8_t *)&(addr))
#define REGW(addr) (*(volatile uint16_t *)&(addr))
#define REGL(addr) (*(volatile uint32_t *)&(addr))

#define BTST(x, b) ((x) & BIT(b))
#define BSET(x, b) ((x) |= BIT(b))
#define BCLR(x, b) ((x) &= ~BIT(b))

#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#define rounddown(x, y) (((x) / (y)) * (y))

/* Wrapper for various GCC attributes */
#define __always_inline __attribute__((always_inline))
#define __noinline __attribute__((noinline))
#define __nonnull(x) __attribute__((nonnull(x)))
#define __noreturn __attribute__((noreturn))
#define __returns_twice __attribute__((returns_twice))
#define __packed __attribute__((packed))
#define __unused __attribute__((unused))
#define __datachip __attribute__((section(".datachip")))
#define __bsschip __attribute__((section(".bsschip")))
#define __aligned(x) __attribute__((aligned(x)))

#define __weak_alias(alias, sym)                                               \
  __asm(".weak " #alias "\n" #alias " = " #sym)
#define __strong_alias(alias, sym)                                             \
  __asm(".global " #alias "\n" #alias " = " #sym)

/* To be used when an empty body is required. */
#define __nothing ((void)0)
