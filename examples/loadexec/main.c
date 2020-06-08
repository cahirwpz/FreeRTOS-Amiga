#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <custom.h>
#include <file.h>
#include <stdio.h>
#include "usermode.h"

extern char _binary_test_exe_end[];
extern char _binary_test_exe_size[];
extern char _binary_test_exe_start[];

static void vMainTask(__unused void *data) {
  File_t *exe =
    MemoryOpen(_binary_test_exe_start, (size_t)_binary_test_exe_size);

  int rv = RunProgram(exe, 2048);

  printf("Program returned: %d\n", rv);

  vTaskDelete(NULL);
}

static xTaskHandle handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  xTaskCreate(vMainTask, "main", configMINIMAL_STACK_SIZE, NULL, 0, &handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
  custom.color[0] = 0x00f;
}
