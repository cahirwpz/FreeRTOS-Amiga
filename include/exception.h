#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

#ifndef ISR_t
#define ISR_t _ISR_t
typedef void (*_ISR_t)(void);
#endif

/* Interrupt Service Routine */
#define ISR(name) __interrupt void name(void)

#define EV_BUSERR 2             /* 2: bus error */
#define EV_ADDRERR 3            /* 3: address error */
#define EV_ILLEGAL 4            /* 4: illegal instruction */
#define EV_ZERODIV 5            /* 5: zero divide */
#define EV_CHK 6                /* 6: CHK instruction */
#define EV_TRAPV 7              /* 7: TRAPV instruction */
#define EV_PRIV 8               /* 8: privilege violation */
#define EV_TRACE 9              /* 9: trace */
#define EV_LINEA 10             /* 10: line 1010 emulator */
#define EV_LINEF 11             /* 11: line 1111 emulator */
#define EV_FMTERR 14            /* 14: format error (68010 ONLY) */
#define EV_BADIVEC 15           /* 15: uninitialized interrupt vector */
#define EV_SPURINT 24           /* 24: spurious interrupt */
#define EV_INTLVL(i) ((i) + 24) /* 25-31: level 1-7 interrupt autovector */
#define EV_TRAP(i) ((i) + 32)   /* 32-47: TRAP 0-15 instruction vector */
#define EV_LAST 255

typedef ISR_t ExcVec_t[EV_LAST + 1];
extern ExcVec_t *ExcVecBase;
#define ExcVec (*ExcVecBase)

#endif /* !_EXCEPTION_H_ */
