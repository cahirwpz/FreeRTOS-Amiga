#include <FreeRTOS.h>
#include <task.h>

#include <hardware.h>
#include <exception.h>
#include <interrupt.h>
#include <trap.h>
#include <cpu.h>

extern void vPortStartFirstTask(void);
extern ISR(vPortYieldHandler);

/* Define custom chipset register bases uses throughout the code. */
volatile Custom_t custom = (Custom_t)0xdff000;
volatile CIA_t ciaa = (CIA_t)0xbfe001;
volatile CIA_t ciab = (CIA_t)0xbfd000;

/* Exception Vector Base: 0 for 68000, for 68010 and above read from VBR */
ExcVec_t *ExcVecBase = (ExcVec_t *)0L;

/* Value of this variable is provided by the boot loader. */
uint8_t CpuModel = 0;

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

void vPortSetupExceptionVector(void) {
  if (CpuModel & CF_68010)
    ExcVecBase = portGetVBR();

  /* Initialize M68k interrupt vector. */
  for (int i = EV_BUSERR; i <= EV_LAST; i++)
    ExcVec[i] = BadTrap;

  /* Initialize exception handlers. */
  ExcVec[EV_BUSERR] = BusErrTrap;
  ExcVec[EV_ADDRERR] = AddrErrTrap;
  ExcVec[EV_ILLEGAL] = IllegalTrap;
  ExcVec[EV_ZERODIV] = ZeroDivTrap;
  ExcVec[EV_CHK] = ChkInstTrap;
  ExcVec[EV_TRAPV] = TrapvInstTrap;
  ExcVec[EV_PRIV] = PrivInstTrap;
  ExcVec[EV_TRACE] = TraceTrap;
  ExcVec[EV_LINEA] = IllegalTrap;
  ExcVec[EV_LINEF] = IllegalTrap;
  ExcVec[EV_FMTERR] = FmtErrTrap;

  /* Initialize level 1-7 interrupt autovector in Amiga specific way. */
  ExcVec[EV_INTLVL(1)] = AmigaLvl1Handler;
  ExcVec[EV_INTLVL(2)] = AmigaLvl2Handler;
  ExcVec[EV_INTLVL(3)] = AmigaLvl3Handler;
  ExcVec[EV_INTLVL(4)] = AmigaLvl4Handler;
  ExcVec[EV_INTLVL(5)] = AmigaLvl5Handler;
  ExcVec[EV_INTLVL(6)] = AmigaLvl6Handler;

  /* Intialize TRAP instruction handlers. */
  for (int i = EV_TRAP(0); i <= EV_TRAP(15); i++)
    ExcVec[i] = IllegalTrap;
}
