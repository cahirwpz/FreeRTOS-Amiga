#include <FreeRTOS.h>
#include <task.h>

#include <hardware.h>
#include <evec.h>
#include <libsa.h>

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

ISR(vDummyExceptionHandler) { portHALT(); }

static ISR(lvl3intr) {
  /* Clear the interrupt. */
  custom->intreq = INTF_VERTB;

  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  BaseType_t xSwitchRequired = xTaskIncrementTick();
  portYIELD_FROM_ISR(xSwitchRequired);
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

void vApplicationSetupInterrupts(void) {
  ExcVec[EV_INTLVL(3)] = lvl3intr;
  custom->intena = INTF_SETCLR|INTF_INTEN|INTF_VERTB;
}

static xTaskHandle red_handle;
static xTaskHandle green_handle;

int main(void) {
  xTaskCreate(vRedTask, "red", configMINIMAL_STACK_SIZE, NULL,
              mainRED_TASK_PRIORITY, &red_handle);

  xTaskCreate(vGreenTask, "green", configMINIMAL_STACK_SIZE, NULL,
              mainGREEN_TASK_PRIORITY, &green_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) { custom->color[0] = 0x00f; }

void vApplicationMallocFailedHook(void) {
  dprintf("Memory exhausted!\n");
  portHALT();
}
