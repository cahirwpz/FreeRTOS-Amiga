#ifndef _CDEFS_H_
#define _CDEFS_H_

#include <stdbool.h>
#include <stdint.h>

#define BIT(x) (1L << (x))

#define REGB(addr) (*(volatile uint8_t *)&(addr))
#define REGW(addr) (*(volatile uint16_t *)&(addr))
#define REGL(addr) (*(volatile uint32_t *)&(addr))

#define BTST(x, b) ((x) & BIT(b))
#define BSET(x, b) ((x) |= BIT(b))
#define BCLR(x, b) ((x) &= ~BIT(b))

#define __unused __attribute__((unused))
#define __datachip __attribute__((section(".datachip")))
#define __bsschip __attribute__((section(".bsschip")))

#endif /* !_CDEFS_H_ */
