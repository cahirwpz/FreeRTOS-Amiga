#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/*---------------------------------------------------------------------------*
 * Application specific definitions.                                         *
 *                                                                           *
 * These definitions should be adjusted for your particular hardware and     *
 * application requirements.                                                 *
 *                                                                           *
 * THESE PARAMETERS ARE DESCRIBED WITHIN THE 'CONFIGURATION' SECTION OF THE  *
 * FreeRTOS API DOCUMENTATION AVAILABLE ON THE FreeRTOS.org WEB SITE.        *
 *                                                                           *
 * See http://www.freertos.org/a00110.html.                                  *
 *---------------------------------------------------------------------------*/

#define configUSE_PREEMPTION            1
#define configUSE_IDLE_HOOK             1
#define configUSE_TICK_HOOK             0
#define configCPU_CLOCK_HZ              ((uint32_t)F_CPU)
#define configTICK_RATE_HZ              ((portTickType)50)
#define configMINIMAL_STACK_SIZE        ((size_t)256)
#define configTOTAL_HEAP_SIZE           ((size_t)65536)
#define configMAX_TASK_NAME_LEN	        (8)
#define configUSE_TASK_NOTIFICATIONS    1
#define configUSE_TRACE_FACILITY        0
#define configUSE_16_BIT_TICKS          0
#define configIDLE_SHOULD_YIELD         1
#define configQUEUE_REGISTRY_SIZE       0
#define configUSE_CO_ROUTINES           0
#define configUSE_MUTEXES               0
#define configCHECK_FOR_STACK_OVERFLOW	1
#define configUSE_RECURSIVE_MUTEXES     0
#define configQUEUE_REGISTRY_SIZE       0
#define configUSE_COUNTING_SEMAPHORES   0

#define configMAX_PRIORITIES            (4)
#define configSUPPORT_STATIC_ALLOCATION  0
#define configSUPPORT_DYNAMIC_ALLOCATION 1
#define configUSE_MALLOC_FAILED_HOOK     1

/* Set the following definitions to 1 to include the API function, or zero
to exclude the API function. */

#define INCLUDE_xTaskGetCurrentTaskHandle       1
#define INCLUDE_vTaskPrioritySet                0
#define INCLUDE_uxTaskPriorityGet               0
#define INCLUDE_vTaskDelete                     1
#define INCLUDE_vTaskCleanUpResources           0
#define INCLUDE_vTaskSuspend                    1
#define INCLUDE_vTaskDelayUntil                 1
#define INCLUDE_vTaskDelay                      1
#define INCLUDE_uxTaskGetStackHighWaterMark     1
#define INCLUDE_xTaskGetSchedulerState          1

/* m68k port specific definitions and options. */
#define portCRITICAL_NESTING_IN_TCB             1

/* What to do when assertion fails? */
#if 1 /* Replace with 0 to turn of verbose assertion messages. */
#define configASSERT(x)                                                        \
  {                                                                            \
    if (!(x))                                                                  \
      portHALT();                                                              \
  }
#else
#include <stdio.h>
#define configASSERT(x)                                                        \
  {                                                                            \
    if (!(x))                                                                  \
    { printf("Error at %s:%d!", __FILE__, __LINE__); portHALT(); }                                                              \
  }
#endif

/*---------------------------------------------------------------------------*
 * Amiga port specific definitions.                                          *
 *---------------------------------------------------------------------------*/

struct TrapFrame;
struct MemRegion;
struct BootData;

/* Called when a synchronous exception or trap happens. */
void vPortTrapHandler(struct TrapFrame *);

/* For use by startup code. */
void vPortDefineMemoryRegions(struct MemRegion *);
void vPortSetupExceptionVector(struct BootData *);

/* Allocate chip memory, should be freed with vPortFree. */
void *pvPortMallocChip(size_t size);

#endif /* FREERTOS_CONFIG_H */
