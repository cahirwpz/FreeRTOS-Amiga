#include <FreeRTOS.h>
#include <task.h>

#include <hardware.h>

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
