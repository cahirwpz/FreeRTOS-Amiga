#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <custom.h>
#include <serial.h>
#include <file.h>
#include <proc.h>
#include <libkern.h>

static File_t *FloppyOpen(const char *path) {
  (void)path;
  return NULL;
}

static void vMainTask(__unused void *data) {
  File_t *ser = SerialOpen(9600);
  File_t *init = FloppyOpen("init");

  Proc_t p;
  ProcInit(&p, UPROC_STKSZ);
  TaskSetProc(&p);
  ProcFileInstall(&p, 0, FileHold(ser));
  ProcFileInstall(&p, 1, FileHold(ser));
  if (ProcLoadImage(&p, init)) {
    ProcSetArgv(&p, (char *[]){"init", NULL});
    ProcEnter(&p);
    kprintf("Program returned: %d\n", p.exitcode);
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
