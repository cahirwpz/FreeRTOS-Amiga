#ifndef _INTERRUPT_H_
#define _INTERRUPT_H_

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

#ifndef __ASSEMBLER__

#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/list.h>

#include <custom.h>

/* All macros below take or'ed INTF_* flags. */
static inline void EnableINT(uint16_t x) { custom.intena_ = INTF_SETCLR | x; }
static inline void DisableINT(uint16_t x) { custom.intena_ = x; }
static inline void CauseIRQ(uint16_t x) { custom.intreq_ = INTF_SETCLR | x; }
static inline void ClearIRQ(uint16_t x) { custom.intreq_ = x; }

/* Interrupt Service Routine */
typedef void (*ISR_t)(void *);

/* Interrupt Vector Entry */
typedef struct IntVecEntry {
  ISR_t code;
  void *data;
} IntVecEntry_t;

typedef IntVecEntry_t IntVec_t[INTB_INTEN];

extern IntVec_t IntVec;

/* Only returns from interrupt, without clearing pending flags. */
extern void DummyInterruptHandler(void *);

/* Macros for setting up ISR for given interrupt number. */
#define SetIntVec(INTR, CODE, DATA)                                            \
  IntVec[INTB_##INTR] = (IntVecEntry_t){.code = (CODE), .data = (DATA)}
#define ResetIntVec(INTR) SetIntVec(INTR, DummyInterruptHandler, NULL)

/* Amiga Interrupt Autovector handlers */
extern void AmigaLvl1Handler(void);
extern void AmigaLvl2Handler(void);
extern void AmigaLvl3Handler(void);
extern void AmigaLvl4Handler(void);
extern void AmigaLvl5Handler(void);
extern void AmigaLvl6Handler(void);

/* Use Interrupt Server to run more than one interrupt handler procedure
 * for given Interrupt Vector routine (VERTB by default). */
typedef void (*IntFunc_t)(void *);

/* Reuses following fields of ListItem_t:
 *  - (BaseType_t) pvOwner: data provided for IntSrvFunc_t,
 *  - (TickType_t) xItemValue: priority of interrupt server. */
typedef struct IntServer {
  ListItem_t node;
  IntFunc_t code;
} IntServer_t;

/* List of interrupt servers. */
typedef struct IntChain {
  List_t list;
  uint16_t flag; /* interrupt enable/disable flag (INTF_*) */
} IntChain_t;

/* Define Interrupt Server to be used with (Add|Rem)IntServer.
 * Priority is between -128 (lowest) to 127 (highest).
 * Because servers are sorted by ascending number of priority,
 * IntServer definition recalculates priority number accordingly.
 */
#define INTSERVER(PRI, CODE, DATA)                                             \
  {.node = {.pvOwner = DATA, .xItemValue = (127 - (PRI))}, .code = CODE} 
#define INTSERVER_DEFINE(NAME, PRI, CODE, DATA)                                \
  static IntServer_t *NAME = &(IntServer_t)INTSERVER(PRI, CODE, DATA)

/* Defines Interrupt Chain of given name. */
#define INTCHAIN(NAME)                                                         \
  IntChain_t *NAME = &(IntChain_t) { 0 }

/* Register Interrupt Server for given Interrupt Chain. */
void AddIntServer(IntChain_t *, IntServer_t *);

/* Unregister Interrupt Server for given Interrupt Chain. */
void RemIntServer(IntServer_t *);

/* Initialize Interrupt Chain structure. */
#define InitIntChain(CHAIN, NUM)                                               \
  {                                                                            \
    vListInitialise(&(CHAIN)->list);                                           \
    (CHAIN)->flag = INTF(NUM);                                                 \
  }

/* Run Interrupt Servers for given Interrupt Chain.
 * Use only inside IntVec handler rountine. */
void RunIntChain(IntChain_t *chain);

/* Predefined interrupt chains defined by Amiga port. */
extern IntChain_t *PortsChain;
extern IntChain_t *VertBlankChain;
extern IntChain_t *ExterChain;

#endif /* __ASSEMBLER__ */

#endif /* !_INTERRUPT_H_ */
