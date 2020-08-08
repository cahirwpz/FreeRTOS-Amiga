#ifndef _CDEFS_H_
#define _CDEFS_H_

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

#define BIT(x) (1L << (x))

#define REGB(addr) (*(volatile uint8_t *)&(addr))
#define REGW(addr) (*(volatile uint16_t *)&(addr))
#define REGL(addr) (*(volatile uint32_t *)&(addr))

#define BTST(x, b) ((x) & BIT(b))
#define BSET(x, b) ((x) |= BIT(b))
#define BCLR(x, b) ((x) &= ~BIT(b))

#define roundup(x, y) ((((x) + ((y) - 1)) / (y)) * (y))
#define rounddown(x, y) (((x) / (y)) * (y))

#define __always_inline __attribute__((always_inline))
#define __noinline __attribute__((noinline))
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

#endif /* !_CDEFS_H_ */
