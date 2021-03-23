#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/semphr.h>

#include <floppy.h>
#include <interrupt.h>
#include <libkern.h>
#include <serial.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#define DEBUG 1
#include <debug.h>

static void vPlusTask(File_t *ser) {
  for (;;)
    kfputchar(ser, '-');
}

static void vMinusTask(File_t *ser) {
  for (;;)
    kfputchar(ser, '+');
}

#define ALLSECS (NTRACKS * NSECTORS)
#define FIRSTSEC (ALLSECS / 2)
#define LASTSEC (ALLSECS - 1)

static uint32_t SectorCksum[ALLSECS];
static SemaphoreHandle_t SectorCksumLock;

static uint32_t FastRand(void) {
  static uint32_t x = 0xDEADC0DE;
  /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

extern uint32_t crc32(const uint8_t *frame, size_t frame_len);

static void vReaderTask(File_t *fd) {
  void *data = pvPortMalloc(SECTOR_SIZE);

  for (;;) {
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    for (short s = FIRSTSEC; s <= LASTSEC; s++) {
      uint32_t cksum;
      xSemaphoreTake(SectorCksumLock, portMAX_DELAY);
      cksum = SectorCksum[s];
      if (cksum) {
        kfseek(fd, s * SECTOR_SIZE, SEEK_SET);
        kfread(fd, data, SECTOR_SIZE);
      }
      xSemaphoreGive(SectorCksumLock);

      if (cksum) {
        uint32_t cksum2 = crc32((void *)data, SECTOR_SIZE);

        DPRINTF("rd(%d/%d): cksum = %08x (memory), cksum = %08x (disk)\n",
                s / NSECTORS, s % NSECTORS, cksum, cksum2);

        DASSERT(cksum == cksum2);

        vTaskDelay(250 / portTICK_PERIOD_MS);
      }
    }
  }
}

static void vWriterTask(File_t *fd) {
  uint32_t *data = pvPortMalloc(SECTOR_SIZE);
  DASSERT(data != NULL);

  for (;;) {
    /* Assume we can freely overwrite second half of floppy disk. */
    short s = FastRand() % (ALLSECS / 2) + ALLSECS / 2;

    /* Fill in sector buffer with random data. */
    for (short i = 0; i < (int)(SECTOR_SIZE / sizeof(uint32_t)); i++)
      data[i] = FastRand();

    xSemaphoreTake(SectorCksumLock, portMAX_DELAY);
    SectorCksum[s] = 0;
    kfseek(fd, s * SECTOR_SIZE, SEEK_SET);
    kfwrite(fd, data, SECTOR_SIZE);
    xSemaphoreGive(SectorCksumLock);

    uint32_t cksum = crc32((void *)data, SECTOR_SIZE);
    DPRINTF("wr(%d/%d): cksum = %08x\n", s / NSECTORS, s % NSECTORS, cksum);
    SectorCksum[s] = cksum;

    /* Wait one second and repeat. */
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

#define FLOPPY_TASK_PRIO 3
#define READER_TASK_PRIO 2
#define WRITER_TASK_PRIO 1
#define PLUS_TASK_PRIO 0
#define MINUS_TASK_PRIO 0

static TaskHandle_t plusHandle;
static TaskHandle_t minusHandle;
static TaskHandle_t readerHandle;
static TaskHandle_t writerHandle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  AddIntServer(VertBlankChain, SystemClockTick);

  SerialInit(9600);
  FloppyInit(FLOPPY_TASK_PRIO);

  File_t *ser = kopen("serial", O_RDWR);
  File_t *fd = kopen("floppy", O_RDWR);

  SectorCksumLock = xSemaphoreCreateMutex();

  memset(SectorCksum, 0, sizeof(SectorCksum));

  xTaskCreate((TaskFunction_t)vPlusTask, "plus", configMINIMAL_STACK_SIZE, ser,
              PLUS_TASK_PRIO, &plusHandle);
  xTaskCreate((TaskFunction_t)vMinusTask, "minus", configMINIMAL_STACK_SIZE,
              ser, MINUS_TASK_PRIO, &minusHandle);
  xTaskCreate((TaskFunction_t)vReaderTask, "reader", configMINIMAL_STACK_SIZE,
              fd, READER_TASK_PRIO, &readerHandle);
  xTaskCreate((TaskFunction_t)vWriterTask, "writer", configMINIMAL_STACK_SIZE,
              fd, WRITER_TASK_PRIO, &writerHandle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
  custom.color[0] = 0;
}
