#include <FreeRTOS/FreeRTOS.h>

#include <floppy.h>

static void SendIO(FloppyIO_t *io, short track) {
  io->track = track;
  FloppySendIO(io);
}

static void WaitIO(QueueHandle_t replyQ, void *buf) {
  FloppyIO_t *io = NULL;
  (void)xQueueReceive(replyQ, &io, portMAX_DELAY);

  DiskSector_t *sectors[SECTOR_COUNT];
  DecodeTrack(io->buffer, sectors);
  for (int j = 0; j < SECTOR_COUNT; j++)
    DecodeSector(sectors[j], buf + j * SECTOR_SIZE);
}

void vReaderTask(void) {
  QueueHandle_t replyQ = xQueueCreate(2, sizeof(FloppyIO_t *));
  void *buf = pvPortMalloc(SECTOR_SIZE * SECTOR_COUNT);

  /*
   * For double buffering we need (sic!) two track buffers:
   *  - one track will be owned by floppy driver
   *    which will set up a DMA transfer to it
   *  - the track will be decoded from MFM format
   *    and possibly read by this task
   */
  FloppyIO_t io[2];
  for (short i = 0; i < 2; i++) {
    io[i].cmd = CMD_READ;
    io[i].buffer = AllocTrack();
    io[i].replyQueue = replyQ;
  }

  for (;;) {
    short track = 0;
    short active = 0;

    /* Initiate double buffered reads. */
    SendIO(&io[active], track++);
    active ^= 1;

    do {
      /* Request asynchronous read into second buffer. */
      SendIO(&io[active], track++);
      active ^= 1;
      /* Wait for reply with first buffer and decode it. */
      WaitIO(replyQ, buf);
    } while (track < TRACK_COUNT);

    /* Finish last track. */
    WaitIO(replyQ, buf);

    /* Wait two seconds and repeat. */
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}
