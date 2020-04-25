#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <stdio.h>
#include <serial.h>

#include "filesys.h"

#define FOREGROUND_TASK_PRIO 0
#define BACKGROUND_TASK_PRIO 0

static void vForegroundTask(File_t *ser) {
  CreateFsReplyQueue();

  for (;;)
    FilePutChar(ser, '-');

  DeleteFsReplyQueue();
}

static void vBackgroundTask(File_t *ser) {
  CreateFsReplyQueue();

  for (;;)
    FilePutChar(ser, '+');
  
  DeleteFsReplyQueue();
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

static xTaskHandle fg_handle;
static xTaskHandle bg_handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  AddIntServer(VertBlankChain, SystemClockTick);

  File_t *ser = SerialOpen(9600);

  xTaskCreate((TaskFunction_t)vForegroundTask, "foreground",
              configMINIMAL_STACK_SIZE, ser, FOREGROUND_TASK_PRIO, &fg_handle);

  xTaskCreate((TaskFunction_t)vBackgroundTask, "background",
              configMINIMAL_STACK_SIZE, ser, BACKGROUND_TASK_PRIO, &bg_handle);

  FsInit();

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) { custom.color[0] = 0; }
