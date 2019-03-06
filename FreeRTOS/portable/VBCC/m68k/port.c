/*
 * FreeRTOS Kernel V10.2.0
 * Copyright (C) 2019 Amazon.com, Inc. or its affiliates.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * http://www.FreeRTOS.org
 * http://aws.amazon.com/freertos
 *
 * 1 tab == 4 spaces!
 */

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

#define portINITIAL_FORMAT_VECTOR ((uint16_t)0x0000)

/* Supervisor mode set. */
#define portINITIAL_STATUS_REGISTER ((uint16_t)0x2000)

/* Used to keep track of the number of nested calls to taskENTER_CRITICAL().
This will be set to 0 prior to the first task being started. */
static uint32_t ulCriticalNesting = 0x9999UL;

#define MOVEL(v)                        \
  *(uint32_t *)sp = (uint32_t)(v)

#define PUSHL(v)                        \
  sp -= sizeof(uint32_t);               \
  *(uint32_t *)sp = (uint32_t)(v);

#define PUSHW(v)                        \
  sp -= sizeof(uint16_t);               \
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

  sp -= 15 * sizeof(uint32_t);  /* A6 to D0. */

  return (StackType_t *)sp;
}

BaseType_t xPortStartScheduler(void) {
  extern void vPortStartFirstTask(void);

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

void vPortYieldHandler(void) {
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  vTaskSwitchContext();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}
