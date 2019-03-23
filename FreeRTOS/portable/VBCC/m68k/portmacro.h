#ifndef PORTMACRO_H
#define PORTMACRO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Type definitions. */
#define portCHAR char
#define portFLOAT float
#define portDOUBLE double
#define portLONG long
#define portSHORT short
#define portSTACK_TYPE uint32_t
#define portBASE_TYPE long

typedef portSTACK_TYPE StackType_t;
typedef long BaseType_t;
typedef unsigned long UBaseType_t;

#if (configUSE_16_BIT_TICKS == 1)
typedef uint16_t TickType_t;
#define portMAX_DELAY (TickType_t)0xffff
#else
typedef uint32_t TickType_t;
#define portMAX_DELAY (TickType_t)0xffffffffUL
#endif

/* VBCC does not understand common way to mark unused variables,
 * i.e. "(void)x", and it complains about efectless statement.
 * Following macros invocations silence out those warnings. */
#define portUNUSED(x)
#define portSETUP_TCB(pxTCB)
#define portCLEAN_UP_TCB(pxTCB)

/* Hardware specifics. */
#define portBYTE_ALIGNMENT 4
#define portSTACK_GROWTH -1
#define portTICK_PERIOD_MS ((TickType_t)1000 / configTICK_RATE_HZ)

/* When code executes in task context it's running with IPL set to 0. */
void portDISABLE_INTERRUPTS() = "\tor.w\t#$0700,sr\n";
void portENABLE_INTERRUPTS() = "\tand.w\t#$f8ff,sr\n";

/* Functions that enter/exit critical section protecting against interrupts. */
void vTaskEnterCritical(void);
void vTaskExitCritical(void);

#define portENTER_CRITICAL() vTaskEnterCritical()
#define portEXIT_CRITICAL() vTaskExitCritical()

/* When code executes in ISR context it may be interrupted on M68000
 * by higher priority level interrupt. To construct critical section
 * we need to use mask bits in SR register. */
uint32_t ulPortSetIPL(__reg("d0") uint32_t);

#define portSET_INTERRUPT_MASK_FROM_ISR() \
  ulPortSetIPL(0x0700)
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedStatusRegister)               \
  ulPortSetIPL(uxSavedStatusRegister)

/* When simulator is configured to enter debugger on illegal instructions,
 * this macro can be used to set breakpoints in your code. */
void portBREAK(void) = "\tillegal\n";

/* Make the processor wait for interrupt. */
void portWFI(void) = "\tstop\t#$2000\n";

/* Halt the processor by masking all interrupts and waiting for NMI. */
void portHALT(void) = "\tstop\t#$2700\n";

/* Read Vector Base Register (68010 and above only) */
void *portGetVBR() = "\tmovec\tvbr,d0\n";

/* Read whole Status Register (privileged instruction on 68010 and above) */
uint16_t portGetSR() = "\tmove.w\tsr,d0\n";

/* What to do when assertion fails? */
#define configASSERT(x)                                                        \
  {                                                                            \
    if (!(x))                                                                  \
      portHALT();                                                              \
  }

/* To yield we use system call that is invoked by TRAP instruction. */
void vPortYield(void) = "\ttrap\t#0\n";

#define portYIELD() vPortYield()
#define portEND_SWITCHING_ISR(xSwitchRequired)                                 \
  {                                                                            \
    if (xSwitchRequired) {                                                     \
      portYIELD();                                                             \
    }                                                                          \
  }
#define portYIELD_FROM_ISR(x) portEND_SWITCHING_ISR(x)

/* Task function macros as described on the FreeRTOS.org WEB site. */
#define portTASK_FUNCTION_PROTO(vFunction, pvParameters)                       \
  void vFunction(void *pvParameters)
#define portTASK_FUNCTION(vFunction, pvParameters)                             \
  void vFunction(void *pvParameters)

#ifdef __cplusplus
}
#endif

#endif /* PORTMACRO_H */
