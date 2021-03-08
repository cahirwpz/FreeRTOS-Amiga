#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <libkern.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <floppy.h>

#define SECTOR(dectrk, i) ((void *)(dectrk) + (i)*SECTOR_SIZE)

static void DoIO(FloppyIO_t *io, uint16_t cmd, uint16_t track) {
  xQueueHandle replyQ = io->replyQueue;
  io->cmd = cmd;
  io->track = track;
  FloppySendIO(io);
  (void)xQueueReceive(replyQ, &io, portMAX_DELAY);
}

static uint32_t rand(void) {
  static uint32_t x = 0xDEADC0DE;
  /* Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs" */
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x;
}

static void SectorFillRandom(uint32_t *decsec) {
  for (short i = 0; i < (int)(SECTOR_SIZE / sizeof(uint32_t)); i++)
    decsec[i] = rand();
}

static void CompareSectors(int trknum, uint32_t *saveTrk, uint32_t *readTrk) {
  for (short i = 0; i < SECTOR_COUNT; i++) {
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

void vWriterTask(void) {
  FloppyIO_t io = {.buffer = AllocTrack(),
                   .replyQueue = xQueueCreate(1, sizeof(FloppyIO_t *))};

  configASSERT(io.buffer);
  configASSERT(io.replyQueue);

  uint32_t *saveTrk = pvPortMalloc(SECTOR_COUNT * SECTOR_SIZE);
  configASSERT(saveTrk);
  uint32_t *readTrk = pvPortMalloc(SECTOR_COUNT * SECTOR_SIZE);
  configASSERT(readTrk);

  DiskSector_t *sectors[SECTOR_COUNT];

  for (;;) {
    /* Assume we can freely overwrite second half of floppy disk. */
    short trknum = rand() % (TRACK_COUNT / 2) + TRACK_COUNT / 2;

    /* Read encoded track but don't decode sectors.
     * We're going to reuse sector headers recorded on the track. */
    DoIO(&io, CMD_READ, trknum);
    DecodeTrack(io.buffer, sectors);

    /* Fill in some of decoded sectors buffer with random data. */
    for (short i = 0; i < SECTOR_COUNT; i++) {
      uint32_t *decsec = SECTOR(saveTrk, i);
      if (rand() % 2) {
        DecodeSector(sectors[i], decsec);
      } else {
        SectorFillRandom(decsec);
        EncodeSector(decsec, sectors[i]);
      }
    }

    /* Before a track is written to disk we need to realign it
     * and fix MFM encoding. */
    RealignTrack(io.buffer, sectors);
    DoIO(&io, CMD_WRITE, trknum);

    /* Read the track from disk. */
    DoIO(&io, CMD_READ, trknum);
    DecodeTrack(io.buffer, sectors);
    for (int j = 0; j < SECTOR_COUNT; j++)
      DecodeSector(sectors[j], SECTOR(readTrk, j));

    CompareSectors(trknum, saveTrk, readTrk);

    /* Wait one second and repeat. */
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }
}
