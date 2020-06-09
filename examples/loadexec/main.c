#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <custom.h>
#include <file.h>
#include <stdio.h>
#include "proc.h"

extern char _binary_shell_exe_end[];
extern char _binary_shell_exe_size[];
extern char _binary_shell_exe_start[];

static void vMainTask(__unused void *data) {
  File_t *shell =
    MemoryOpen(_binary_shell_exe_start, (size_t)_binary_shell_exe_size);

  Proc_t p;
  ProcInit(&p, UPROC_STKSZ);

  int rv = Execute(&p, shell, (char *[]){"shell", NULL});

  printf("Program returned: %d\n", rv);

  ProcFini(&p);

  vTaskDelete(NULL);
}

static xTaskHandle handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  xTaskCreate(vMainTask, "main", KPROC_STKSZ, NULL, 0, &handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
  custom.color[0] = 0x00f;
}
