#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <stdio.h>

#define mainRED_TASK_PRIORITY 3
#define mainGREEN_TASK_PRIORITY 3

static void vRedTask(__unused void *data) {
  for (;;) {
    custom.color[0] = 0xf00;
  }
}

static void vGreenTask(__unused void *data) {
  for (;;) {
    custom.color[0] = 0x0f0;
  }
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

static xTaskHandle red_handle;
static xTaskHandle green_handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  AddIntServer(VertBlankChain, SystemClockTick);

  xTaskCreate(vRedTask, "red", configMINIMAL_STACK_SIZE, NULL,
              mainRED_TASK_PRIORITY, &red_handle);

  xTaskCreate(vGreenTask, "green", configMINIMAL_STACK_SIZE, NULL,
              mainGREEN_TASK_PRIORITY, &green_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) { custom.color[0] = 0x00f; }
