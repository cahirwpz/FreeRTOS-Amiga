#ifndef _CUSTOM_H_
#define _CUSTOM_H_

#include <custom_regdef.h>

extern struct Custom volatile custom;

/* Macros below take or'ed DMAF_* flags. */
static inline void EnableDMA(uint16_t x) { custom.dmacon_ = DMAF_SETCLR | x; }
static inline void DisableDMA(uint16_t x) { custom.dmacon_ = x; }

static inline void WaitLine(uint32_t line) {
  while ((custom.vposr_ & 0x1ff00) != ((line << 8) & 0x1ff00));
}

#endif /* !_CUSTOM_H_ */
