#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <stdio.h>

#include <file.h>

#include "console.h"
#include "event.h"
#include "tty.h"

#define mainINPUT_TASK_PRIORITY 3

static void vInputTask(void *data) {
  File_t *tty = data;
  for (;;) {
    Event_t ev;
    if (!PopEvent(&ev))
      continue;
    if (ev.type == EV_MOUSE) {
      FilePrintf(tty, "MOUSE: x = %d, y = %d, button = %x\n", ev.mouse.x,
                 ev.mouse.y, ev.mouse.button);
      ConsoleMovePointer(ev.mouse.x, ev.mouse.y);
    } else if (ev.type == EV_KEY) {
      FilePrintf(tty, "KEY: ascii = '%c', code = %02x, modifier = %02x\n",
                 ev.key.ascii, ev.key.code, ev.key.modifier);
    }
  }
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

static xTaskHandle input_handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  /* Configure system clock. */
  AddIntServer(VertBlankChain, SystemClockTick);

  ConsoleInit();

  xTaskCreate(vInputTask, "input", configMINIMAL_STACK_SIZE, TtyOpen(),
              mainINPUT_TASK_PRIORITY, &input_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
}
