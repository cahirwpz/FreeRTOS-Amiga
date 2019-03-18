#include <FreeRTOS.h>
#include <task.h>
#include <hardware.h>
#include <evec.h>
#include <cpu.h>

extern void vPortStartFirstTask(void);
extern __interrupt void vPortYieldHandler(void);

/* When calling RTE the stack must look as follows:
 *
 *   +--------+---------------+
 *   | FORMAT | VECTOR OFFSET | (M68010 only)
 *   +--------+---------------+
 *   |  PROGRAM COUNTER LOW   |
 *   +------------------------+
 *   |  PROGRAM COUNTER HIGH  |
 *   +------------------------+
 *   |    STATUS REGISTER     | <-- stack pointer
 *   +------------------------+
 *
 * Remember that stack grows down!
 */

#define portINITIAL_FORMAT_VECTOR ((uint16_t)0x0000)

/* Initial SR for new task: supervisor mode with interrupts unmasked. */
#define portINITIAL_STATUS_REGISTER ((uint16_t)0x2000)

/* Macros for task stack initialization. */
#define MOVEL(v) *(uint32_t *)sp = (uint32_t)(v)

#define PUSHL(v)                                                               \
  sp -= sizeof(uint32_t);                                                      \
  *(uint32_t *)sp = (uint32_t)(v);

#define PUSHW(v)                                                               \
  sp -= sizeof(uint16_t);                                                      \
  *(uint16_t *)sp = (uint16_t)(v);

StackType_t *pxPortInitialiseStack(StackType_t *pxTopOfStack,
                                   TaskFunction_t pxCode, void *pvParameters) {
  char *sp = (char *)pxTopOfStack;

  MOVEL(pvParameters);
  PUSHL(0xDEADBEEF);

  /* Exception stack frame starts with the return address, unless we're running
   * on 68010 and above. Then we need to put format vector word on stack. */
  if (CpuModel & CF_68010)
    PUSHW(portINITIAL_FORMAT_VECTOR);
  PUSHL(pxCode);
  PUSHW(portINITIAL_STATUS_REGISTER);

  sp -= 15 * sizeof(uint32_t); /* A6 to D0. */

  return (StackType_t *)sp;
}

/* It's assumed that we enter here:
 *  - with all Amiga interrupts disabled, i.e. INTENAR = 0,
 *  - at highest priority level, i.e. SR = 0x2700. 
 */
BaseType_t xPortStartScheduler(void) {
  /* Use TRAP #0 for Yield system call. */
  ExcVec[EV_TRAP(0)] = vPortYieldHandler;

  /* Unmask all interrupts in INTENA. They're still masked by SR. */
  custom->intena = INTF_SETCLR|INTF_INTEN;

  /* Start the first task executing. */
  vPortStartFirstTask();

  return pdFALSE;
}

void vPortEndScheduler(void) {
  /* Not implemented as there is nothing to return to. */
}
