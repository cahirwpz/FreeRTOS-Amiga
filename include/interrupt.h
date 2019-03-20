#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <hardware/intbits.h>

#ifndef ISR_t
#define ISR_t _ISR_t
typedef void (*_ISR_t)(void);
#endif

typedef ISR_t IntVec_t[INTB_INTEN];
extern IntVec_t IntVec;

#endif /* !_INTERRUPT_H_ */
