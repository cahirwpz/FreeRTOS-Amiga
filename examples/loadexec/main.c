#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <custom.h>
#include <amigahunk.h>
#include <stdio.h>
#include "memory.h"

extern char _binary_test_exe_end[];
extern char _binary_test_exe_size[];
extern char _binary_test_exe_start[];

static void vMainTask(__unused void *data) {
  File_t *exe = MemoryOpen(_binary_test_exe_start,
                           (size_t)_binary_test_exe_size);

  Hunk_t *first = LoadHunkList(exe);

  for (Hunk_t *hunk = first; hunk; hunk = hunk->next)
    hexdump(hunk->data, hunk->size);

  /* Assume first hunk is executable. */
  void (*_start)(void) = (void *)first->data;
  /* Call _start procedure which is assumed to be first. */
  _start();

  for (;;) {
    custom.color[0] = 0xf00;
  }
}

static xTaskHandle handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  xTaskCreate(vMainTask, "main", configMINIMAL_STACK_SIZE, NULL, 0, &handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) { custom.color[0] = 0x00f; }
