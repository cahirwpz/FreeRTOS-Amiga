#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <stdlib.h>
#include <memory.h>

#include "filesys.h"

#define READER_TASK_NUM 4
#define READER_TASK_PRIO 1

static int DirectorySize(void) {
  DirEntry_t *de = NULL;
  short i = 0;
  while (FsListDir((void **)&de))
    i++;
  return i;
}

static File_t *OpenNthFile(short n) {
  DirEntry_t *de = NULL;
  for (short i = 0; i <= n; i++)
    FsListDir((void **)&de);
  return FsOpen(de->name);
}

#define BUFSIZE 256

static void vReaderTask(__unused void *data) {
  FsMount();

  unsigned seed = (intptr_t)xTaskGetCurrentTaskHandle();
  uint8_t *buf = MemAlloc(BUFSIZE, 0);
  int n = DirectorySize();

  for (;;) {
    File_t *f = OpenNthFile(rand_r(&seed) % n);
    long nbyte;
    do {
      FileRead(f, buf, 1 + rand_r(&seed) % BUFSIZE, &nbyte);
      vTaskDelay((750 + rand_r(&seed) % 250) / portTICK_PERIOD_MS);
    } while (nbyte != 0);
    FileClose(f);
  }
}

static TaskHandle_t readerHandle[READER_TASK_NUM];

void StartReaderTasks(void) {
  for (int i = 0; i < READER_TASK_NUM; i++)
    xTaskCreate((TaskFunction_t)vReaderTask, "reader", configMINIMAL_STACK_SIZE,
                NULL, READER_TASK_PRIO, &readerHandle[i]);
}
