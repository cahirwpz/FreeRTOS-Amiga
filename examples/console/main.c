#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <libkern.h>
#include <file.h>
#include <console.h>

#include "event.h"

#define mainINPUT_TASK_PRIORITY 3

static void vInputTask(void *data) {
  File_t *cons = data;
  for (;;) {
    Event_t ev;
    if (!PopEvent(&ev))
      continue;
    if (ev.type == EV_MOUSE) {
      kfprintf(cons, "MOUSE: x = %d, y = %d, button = %x\n", ev.mouse.x,
               ev.mouse.y, ev.mouse.button);
      ConsoleMovePointer(ev.mouse.x, ev.mouse.y);
    } else if (ev.type == EV_KEY) {
      kfprintf(cons, "KEY: ascii = '%c', code = %02x, modifier = %02x\n",
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

  (void)ConsoleInit();
  EventQueueInit();
  MouseInit(PushMouseEventFromISR, 0, 0, 319, 255);
  KeyboardInit(PushKeyEventFromISR);

  File_t *cons = kopen("console", O_RDWR);

  xTaskCreate(vInputTask, "input", configMINIMAL_STACK_SIZE, cons,
              mainINPUT_TASK_PRIORITY, &input_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
}
