#include <FreeRTOS.h>
#include <task.h>

extern void vPortStartFirstTask(void);

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

  /* Exception stack frame starts with the return address. */
  /* only for 68010 and above
   * PUSHW(portINITIAL_FORMAT_VECTOR); */
  PUSHL(pxCode);
  PUSHW(portINITIAL_STATUS_REGISTER);

  sp -= 15 * sizeof(uint32_t); /* A6 to D0. */

  return (StackType_t *)sp;
}

/* Used to keep track of the number of nested calls to taskENTER_CRITICAL().
 * This will be set to 0 prior to the first task being started. */
static uint32_t ulCriticalNesting = 0x9999UL;

/* It's assumed that we enter here:
 *  - with all Amiga interrupts disabled, i.e. INTENAR = 0,
 *  - at highest priority level, i.e. SR = 0x2700. 
 */
BaseType_t xPortStartScheduler(void) {
  ulCriticalNesting = 0UL;

  /* Configure the interrupts used by this port. */
  vApplicationSetupInterrupts();

  /* Start the first task executing. */
  vPortStartFirstTask();

  return pdFALSE;
}

void vPortEndScheduler(void) {
  /* Not implemented as there is nothing to return to. */
}

void vPortEnterCritical(void) {
  portDISABLE_INTERRUPTS();
  ulCriticalNesting++;
}

void vPortExitCritical(void) {
  ulCriticalNesting--;
  if (ulCriticalNesting == 0)
    portENABLE_INTERRUPTS();
}
