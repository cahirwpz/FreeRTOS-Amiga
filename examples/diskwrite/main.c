#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <stdio.h>
#include <string.h>
#include <serial.h>
#include <floppy.h>
#include <file.h>

#define DRYTEST 0

int rand(void);

static inline void SendIO(FloppyIO_t *io, uint16_t cmd, uint16_t track) {
  io->cmd = cmd;
  io->track = track;
  FloppySendIO(io);
}

static void WaitIO(QueueHandle_t replyQ, void *buf,
                   DiskSector_t *sectors[SECTOR_COUNT]) {
  FloppyIO_t *io = NULL;
  (void)xQueueReceive(replyQ, &io, portMAX_DELAY);
  if (io->cmd == CMD_READ) {
    RealignTrack(io->buffer, sectors);
    for (int j = 0; j < SECTOR_COUNT; j++)
      DecodeSector(sectors[j], buf + j * SECTOR_SIZE);
  }
}

static int MemEqual(const char *m0, const char *m1, size_t len) {
  for (size_t i = 0; i < len; i++)
    if (*m0++ != *m1++)
      return 0;
  return 1;
}

static char sequence[] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9,
                          9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

#define SECTOR(track, i) ((void *)(track) + (i)*SECTOR_SIZE)

static void vTask(__unused File_t *f) {
  DiskTrack_t *chipbuf = AllocTrack();
  configASSERT(chipbuf);

  QueueHandle_t replyQ = xQueueCreate(1, sizeof(FloppyIO_t *));
  ;
  configASSERT(replyQ);

  FloppyIO_t io = {.buffer = chipbuf, .replyQueue = replyQ};

  char *data = pvPortMalloc(SECTOR_COUNT * SECTOR_SIZE);
  configASSERT(data);

  DiskSector_t **sectors = pvPortMalloc(SECTOR_COUNT * sizeof(DiskSector_t *));
  configASSERT(sectors);

  uint32_t good = 0, bad = 0;

  for (;;)
    for (uint16_t track = 1; track < TRACK_COUNT; track++) {
      SendIO(&io, CMD_READ, track);
      WaitIO(replyQ, data, sectors);

      uint16_t secnum = rand() % SECTOR_COUNT;
      uint16_t offset = rand() % (SECTOR_SIZE - sizeof(sequence));
      void *sector = SECTOR(data, secnum);
      memcpy(sector + offset, sequence, sizeof(sequence));
      EncodeSector(sector, sectors[secnum]);
#if DRYTEST
      FixTrackEncoding(chipbuf);
      for (int j = 0; j < SECTOR_COUNT; j++)
        DecodeSector(sectors[j], SECTOR(data, j));
#else
      SendIO(&io, CMD_WRITE, track);
      WaitIO(replyQ, NULL, NULL);
      SendIO(&io, CMD_READ, track);
      WaitIO(replyQ, data, sectors);
#endif
      if (MemEqual(sector + offset, sequence, sizeof(sequence))) {
        FilePutChar(f, '+');
        good++;
      } else {
        FilePutChar(f, '-');
        bad++;
        printf("good = %u, bad = %u\n", good, bad);
        FilePutChar(f, '\n');
      }
    }
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

static TaskHandle_t handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  AddIntServer(VertBlankChain, SystemClockTick);

  File_t *ser = SerialOpen(9600);

  xTaskCreate((TaskFunction_t)vTask, "task", configMINIMAL_STACK_SIZE, ser, 0,
              &handle);

  FloppyInit(3);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
}
