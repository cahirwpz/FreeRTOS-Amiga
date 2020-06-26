#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <custom.h>
#include <serial.h>
#include <file.h>
#include <stdio.h>
#include "proc.h"

extern char _binary_ushell_exe_end[];
extern char _binary_ushell_exe_size[];
extern char _binary_ushell_exe_start[];

extern char _binary_ucat_exe_end[];
extern char _binary_ucat_exe_size[];
extern char _binary_ucat_exe_start[];

static void vMainTask(__unused void *data) {
  File_t *ser = SerialOpen(9600);
  File_t *shell =
    MemoryOpen(_binary_ushell_exe_start, (size_t)_binary_ushell_exe_size);

  Proc_t p;
  ProcInit(&p, UPROC_STKSZ);
  TaskSetProc(&p);
  ProcFileInstall(&p, 0, FileHold(ser));
  ProcFileInstall(&p, 1, FileHold(ser));
  if (ProcLoadImage(&p, shell)) {
    ProcSetArgv(&p, (char *[]){"shell", NULL});
    ProcEnter(&p);
    printf("Program returned: %d\n", p.exitcode);
  }
  ProcFini(&p);

  FileClose(ser);
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
