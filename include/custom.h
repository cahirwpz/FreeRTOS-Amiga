#ifndef _CUSTOM_H_
#define _CUSTOM_H_

#include <custom_regdef.h>

typedef struct Custom *const Custom_t;

extern volatile Custom_t custom;

#define EnableDMA(x) custom->dmacon_ = DMAF_SETCLR | DMAF(x)
#define DisableDMA(x) custom->dmacon_ = DMAF(x)

#endif /* !_CUSTOM_H_ */
