#include <hardware/intbits.h>
#include <hardware/custom.h>

#include <FreeRTOS.h>
#include <task.h>

extern volatile struct Custom* const custom;

#define ISR(name) __interrupt void name(void) 

#define buserr badtrap
#define addrerr badtrap
#define illinst badtrap
#define zerodiv badtrap
#define chkinst badtrap
#define trapvinst badtrap
#define privinst badtrap
#define fmterr badtrap
#define lvl1intr badtrap
#define lvl2intr badtrap
#define lvl4intr badtrap
#define lvl5intr badtrap
#define lvl6intr badtrap
#define lvl7intr badtrap

ISR(badtrap) { for(;;); }

ISR(lvl3intr) {
  /* Clear the interrupt. */
  custom->intreq = INTF_VERTB;

  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  BaseType_t xSwitchRequired = xTaskIncrementTick();
  portYIELD_FROM_ISR(xSwitchRequired);
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

#define VECTOR(handler) *evec++ = (uint32_t)handler

void vApplicationSetupInterrupts(void) {
  uint32_t *evec = (uint32_t *)0; 

  evec++;               /* 0: reset - initial SSP (not used) */
  evec++;               /* 1: reset - initial PC (not used) */
  VECTOR(buserr);       /* 2: bus error */
  VECTOR(addrerr);      /* 3: address error */
  VECTOR(illinst);      /* 4: illegal instruction */
  VECTOR(zerodiv);      /* 5: zero divide */
  VECTOR(chkinst);      /* 6: CHK instruction */
  VECTOR(trapvinst);    /* 7: TRAPV instruction */
  VECTOR(privinst);     /* 8: privilege violation */
  VECTOR(badtrap);      /* 9: trace */
  VECTOR(illinst);      /* 10: line 1010 emulator */
  VECTOR(illinst);      /* 11: line 1111 emulator */
  VECTOR(badtrap);      /* 12: unassigned, reserved */
  VECTOR(badtrap);      /* 13: unassigned, reserved */
  VECTOR(fmterr);       /* 14: format error (68010 ONLY) */
  VECTOR(badtrap);      /* 15: uninitialized interrupt vector */

  for (int i = 0; i < 8; i++)
    VECTOR(badtrap);    /* 16-23: unassigned, reserved */

  VECTOR(badtrap);      /* 24: spurious interrupt */
  VECTOR(lvl1intr);     /* 25: level 2 interrupt autovector */
  VECTOR(lvl2intr);     /* 26: level 2 interrupt autovector */
  VECTOR(lvl3intr);     /* 27: level 3 interrupt autovector */
  VECTOR(lvl4intr);     /* 28: level 4 interrupt autovector */
  VECTOR(lvl5intr);     /* 29: level 5 interrupt autovector */
  VECTOR(lvl6intr);     /* 30: level 6 interrupt autovector */
  VECTOR(lvl7intr);     /* 31: level 7 interrupt autovector */

  for (int i = 0; i < 16; i++)
    VECTOR(illinst);    /* 32-47: TRAP instruction vector */

  for (int i = 0; i < 16; i++)
    VECTOR(badtrap);    /* 48-63: unassigned, reserved */

  for (int i = 0; i < 192; i++)
    VECTOR(badtrap);    /* 64-255: user interrupt vectors */

  custom->intena = INTF_SETCLR|INTF_INTEN|INTF_VERTB;
}
