#ifndef _CUSTOM_H_
#define _CUSTOM_H_

#include <custom_regdef.h>

typedef struct Custom *const Custom_t;

#define CUSTOM ((volatile Custom_t)0xdff000)

extern volatile Custom_t custom;

/* Macros below take or'ed DMAF_* flags. */
#define EnableDMA(x) { custom->dmacon_ = DMAF_SETCLR | (x); }
#define DisableDMA(x) { custom->dmacon_ = (x); }

#endif /* !_CUSTOM_H_ */
