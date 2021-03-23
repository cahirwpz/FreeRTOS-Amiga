#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

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

static void vReaderTask(File_t *fd) {
  void *buf = pvPortMalloc(SECTOR_SIZE);

  for (;;) {
    short sector = 0;

    kfseek(fd, 0, SEEK_SET);

    do {
      kfread(fd, buf, SECTOR_SIZE);
    } while (++sector < NTRACKS * NSECTORS);

    /* Wait two seconds and repeat. */
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

#define SECTOR(dectrk, i) ((void *)(dectrk) + (i)*SECTOR_SIZE)

static uint32_t fastrand(void) {
  static uint32_t x = 0xDEADC0DE;
  /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

static void SectorFillRandom(uint32_t *rawsec) {
  for (short i = 0; i < (int)(SECTOR_SIZE / sizeof(uint32_t)); i++)
    rawsec[i] = fastrand();
}

static void CompareSectors(int trknum, uint32_t *saveTrk, uint32_t *readTrk) {
  for (short i = 0; i < NSECTORS; i++) {
    uint32_t *old = SECTOR(saveTrk, i);
    uint32_t *new = SECTOR(readTrk, i);
    for (short j = 0; j < (int)(SECTOR_SIZE / sizeof(uint32_t)); j++) {
      if (old[j] != new[j]) {
        kprintf("trk(%3d), sec(%2d), off(%3d): %08x (old) vs. %08x (new)\n",
                trknum, i, j * sizeof(uint32_t), old[j], new[j]);
        portBREAK();
        return;
      }
    }
  }
}

static void vWriterTask(File_t *fd __unused) {
  uint32_t *saveTrk = pvPortMalloc(TRACK_SIZE);
  DASSERT(saveTrk != NULL);
  uint32_t *readTrk = pvPortMalloc(TRACK_SIZE);
  DASSERT(readTrk != NULL);

  for (;;) {
    /* Assume we can freely overwrite second half of floppy disk. */
    short trknum = fastrand() % (NTRACKS / 2) + NTRACKS / 2;

    /* Fill in some of decoded sectors buffer with random data. */
    for (short i = 0; i < NSECTORS; i++) {
      uint32_t *rawsec = SECTOR(saveTrk, i);
      if (fastrand() % 2) {
        // DecodeSector(sectors[i], rawsec);
      } else {
        SectorFillRandom(rawsec);
        // EncodeSector(rawsec, sectors[i]);
      }
    }

    /* Read the track from disk. */
    // DoIO(&io, CMD_READ, trknum);
    // DecodeTrack(io.buffer, sectors);
    // for (int j = 0; j < SECTOR_COUNT; j++)
    //  DecodeSector(sectors[j], SECTOR(readTrk, j));

    CompareSectors(trknum, saveTrk, readTrk);

    /* Wait one second and repeat. */
    vTaskDelay(1000 / portTICK_PERIOD_MS);
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
