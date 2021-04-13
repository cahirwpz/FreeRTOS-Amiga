#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <custom.h>
#include <driver.h>
#include <devfile.h>
#include <file.h>
#include <filedesc.h>
#include <proc.h>
#include <libkern.h>
#include <tty.h>

static File_t *FloppyOpen(const char *path) {
  (void)path;
  return NULL;
}

static void vMainTask(__unused void *data) {
  File_t *ser = kopen("tty", O_RDWR);
  File_t *init = FloppyOpen("init");

  Proc_t p;
  ProcInit(&p, UPROC_STKSZ);
  TaskSetProc(&p);
  FdInstallAt(&p, FileHold(ser), 0);
  FdInstallAt(&p, FileHold(ser), 1);
  if (ProcLoadImage(&p, init)) {
    ProcSetArgv(&p, (char *[]){"init", NULL});
    ProcEnter(&p);
    klog("Program returned: %d\n", p.exitcode);
  }
  ProcFini(&p);

  FileClose(ser);
  vTaskDelete(NULL);
}

static xTaskHandle handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  DeviceAttach(&Serial);
  AddTtyDevFile("tty", kopen("serial", O_RDWR));

  xTaskCreate(vMainTask, "main", KPROC_STKSZ, NULL, 0, &handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
  custom.color[0] = 0x00f;
}
