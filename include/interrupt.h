#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

#include <custom.h>

#define INTB_SETCLR 15  /* Set/Clear control bit. Determines if bits */
                        /* written with a one get set or cleared. Bits */
                        /* written with a zero are always unchanged */
#define INTB_INTEN 14   /* Master interrupt (enable only) */
#define INTB_EXTER 13   /* External interrupt */
#define INTB_DSKSYNC 12 /* Disk re-SYNChronized */
#define INTB_RBF 11     /* Serial port Receive Buffer Full */
#define INTB_AUD3 10    /* Audio channel 3 block finished */
#define INTB_AUD2 9     /* Audio channel 2 block finished */
#define INTB_AUD1 8     /* Audio channel 1 block finished */
#define INTB_AUD0 7     /* Audio channel 0 block finished */
#define INTB_BLIT 6     /* Blitter finished */
#define INTB_VERTB 5    /* Start of Vertical Blank */
#define INTB_COPER 4    /* Coprocessor */
#define INTB_PORTS 3    /* I/O Ports and timers */
#define INTB_SOFTINT 2  /* Software interrupt request */
#define INTB_DSKBLK 1   /* Disk Block done */
#define INTB_TBE 0      /* Serial port Transmit Buffer Empty */

#define INTF(x) BIT(INTB_##x)

#define INTF_SETCLR INTF(SETCLR)
#define INTF_INTEN INTF(INTEN)
#define INTF_EXTER INTF(EXTER)
#define INTF_DSKSYNC INTF(DSKSYNC)
#define INTF_RBF INTF(RBF)
#define INTF_AUD3 INTF(AUD3)
#define INTF_AUD2 INTF(AUD2)
#define INTF_AUD1 INTF(AUD1)
#define INTF_AUD0 INTF(AUD0)
#define INTF_BLIT INTF(BLIT)
#define INTF_VERTB INTF(VERTB)
#define INTF_COPER INTF(COPER)
#define INTF_PORTS INTF(PORTS)
#define INTF_SOFTINT INTF(SOFTINT)
#define INTF_DSKBLK INTF(DSKBLK)
#define INTF_TBE INTF(TBE)

#define EnableINT(x) custom->intena_ = INTF_SETCLR | INTF(x)
#define DisableINT(x) custom->intena_ = INTF(x)
#define CauseIRQ(x) custom->intreq_ = INTF_SETCLR | INTF(x)
#define ClearIRQ(x) custom->intreq_ = INTF(x)

#ifndef ISR_t
#define ISR_t _ISR_t
/* Interrupt Service Routine */
typedef void (*_ISR_t)(void);
#define ISR(name) __interrupt void name(void)
#endif

typedef ISR_t *IntVec_t[INTB_INTEN];
extern IntVec_t IntVec;

extern void DummyInterruptHandler(void);

#define SetIntVec(intbit, handler) *(IntVec[INTB_##intbit]) = handler
#define ResetIntVec(intbit) SetIntVec(intbit, DummyInterruptHandler)

/* Amiga Interrupt Autovector handlers */
extern void AmigaLvl1Handler(void);
extern void AmigaLvl2Handler(void);
extern void AmigaLvl3Handler(void);
extern void AmigaLvl4Handler(void);
extern void AmigaLvl5Handler(void);
extern void AmigaLvl6Handler(void);

#endif /* !_INTERRUPT_H_ */
