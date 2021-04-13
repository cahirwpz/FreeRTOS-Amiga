#pragma once

#include <sys/cdefs.h>
#include <cpu.h>
#include <debug.h>

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

#define portFORCE_INLINE __always_inline inline

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
#define portDISABLE_INTERRUPTS()                                               \
  { asm volatile("\tor.w\t#0x0700,%sr\n"); }
#define portENABLE_INTERRUPTS()                                                \
  { asm volatile("\tand.w\t#0xf8ff,%sr\n"); }

/* Functions that enter/exit critical section protecting against interrupts. */
void vTaskEnterCritical(void);
void vTaskExitCritical(void);

#define portENTER_CRITICAL() vTaskEnterCritical()
#define portEXIT_CRITICAL() vTaskExitCritical()

/* When code executes in ISR context it may be interrupted on M68000
 * by higher priority level interrupt. To construct critical section
 * we need to use mask bits in SR register. */
uint32_t ulPortSetIPL(uint32_t);

#define portSET_INTERRUPT_MASK_FROM_ISR() ulPortSetIPL(0x0700)
#define portCLEAR_INTERRUPT_MASK_FROM_ISR(uxSavedStatusRegister)               \
  ulPortSetIPL(uxSavedStatusRegister)

/* To yield we use system call that is invoked by TRAP instruction. */
#define vPortYield() TRAP(0)

#define portYIELD() vPortYield()

/* Fail if caller is NOT running in ISR context. */
#define portASSERT_IF_IN_ISR() configASSERT(portGetSR() & 0x0700)

/* Set to non-zero value inside interrupt service routine
 * whenever you woke up a higher priority task. */
extern BaseType_t xNeedRescheduleTask;

/* Task function macros as described on the FreeRTOS.org WEB site. */
#define portTASK_FUNCTION_PROTO(vFunction, pvParameters)                       \
  void vFunction(void *pvParameters)
#define portTASK_FUNCTION(vFunction, pvParameters)                             \
  void vFunction(void *pvParameters)

#ifdef __cplusplus
}
#endif
