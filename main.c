#include <FreeRTOS.h>
#include <task.h>

#include <interrupt.h>
#include <stdio.h>

#define mainRED_TASK_PRIORITY 3
#define mainGREEN_TASK_PRIORITY 3

static void vRedTask(void *) {
  for (;;) {
    custom->color[0] = 0xf00;
  }
}

static void vGreenTask(void *) {
  for (;;) {
    custom->color[0] = 0x0f0;
  }
}

  }
}

static ISR(VertBlankHandler) {
  /* Clear the interrupt. */
  ClearIRQ(VERTB);

  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  BaseType_t xSwitchRequired = xTaskIncrementTick();
  portYIELD_FROM_ISR(xSwitchRequired);
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

static void SystemTimerInit(void) {
  SetIntVec(VERTB, VertBlankHandler);
  ClearIRQ(VERTB);
  EnableINT(VERTB);
}

static xTaskHandle red_handle;
static xTaskHandle green_handle;

int main(void) {
  SystemTimerInit();

  xTaskCreate(vRedTask, "red", configMINIMAL_STACK_SIZE, NULL,
              mainRED_TASK_PRIORITY, &red_handle);

  xTaskCreate(vGreenTask, "green", configMINIMAL_STACK_SIZE, NULL,
              mainGREEN_TASK_PRIORITY, &green_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) { custom->color[0] = 0x00f; }

void vApplicationMallocFailedHook(void) {
  printf("Memory exhausted!\n");
  portHALT();
}

void vApplicationStackOverflowHook(void) {
  printf("Stack overflow!\n");
  portHALT();
}
