#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <hardware/intbits.h>

#ifndef ISR_t
#define ISR_t _ISR_t
/* Interrupt Service Routine */
typedef void (*_ISR_t)(void);
#define ISR(name) __interrupt void name(void)
#endif

typedef ISR_t *IntVec_t[INTB_INTEN];
extern IntVec_t IntVec;

extern void DummyInterruptHandler(void);

#define SetIntVec(intbit, handler) *(IntVec[INTB_ ## intbit]) = handler
#define ResetIntVec(intbit) SetIntVec(intbit, DummyInterruptHandler)

/* Amiga Interrupt Autovector handlers */
extern void AmigaLvl1Handler(void);
extern void AmigaLvl2Handler(void);
extern void AmigaLvl3Handler(void);
extern void AmigaLvl4Handler(void);
extern void AmigaLvl5Handler(void);
extern void AmigaLvl6Handler(void);

#endif /* !_INTERRUPT_H_ */
