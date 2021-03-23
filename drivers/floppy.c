#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/queue.h>

#include <interrupt.h>
#include <custom.h>
#include <cia.h>

#include <string.h>
#include <libkern.h>
#include <device.h>
#include <ioreq.h>
#include <sys/errno.h>

#define __FLOPPY_DRIVER
#include <floppy.h>

#define DEBUG 1
#include <debug.h>

#define IOREQ_MAXNUM 16

typedef struct FloppyDev {
  CIATimer_t *timer;
  xTaskHandle ioTask;
  QueueHandle_t ioQueue;
  DiskTrack_t *diskTrack;

  int16_t track;   /* track currently stored in `trackBuf` or -1 */
  int16_t motorOn; /* motor is turned on or off */
  int16_t headDir; /* head moves outward on inwards by two tracks */
  int16_t headTrk; /* head is positioned over this track */

  DiskSector_t *diskSector[NSECTORS];
  RawSector_t sector[NSECTORS];
  SectorState_t sectorState[NSECTORS];
} FloppyDev_t;

static FloppyDev_t FloppyDev[1];

static void TrackTransferDone(void *ptr) {
  FloppyDev_t *fd = ptr;
  /* Send notification to waiting task. */
  vTaskNotifyGiveFromISR(fd->ioTask, &xNeedRescheduleTask);
}

static void FloppyReader(void *);
static int FloppyReadWrite(Device_t *, IoReq_t *);

static DeviceOps_t FloppyOps = {.read = FloppyReadWrite,
                                .write = FloppyReadWrite};

Device_t *FloppyInit(unsigned aFloppyIOTaskPrio) {
  FloppyDev_t *fd = FloppyDev;

  kprintf("[Init] Floppy drive driver!\n");

  fd->timer = AcquireTimer(TIMER_ANY);
  DASSERT(fd->timer != NULL);

  /* Set standard synchronization marker. */
  custom.dsksync = DSK_SYNC;

  /* Standard settings for Amiga format disk floppies. */
  custom.adkcon = ADKF_SETCLR | ADKF_MFMPREC | ADKF_WORDSYNC | ADKF_FAST;

  /* Handler that will wake up track reader task. */
  SetIntVec(DSKBLK, TrackTransferDone, fd);

  fd->ioQueue = xQueueCreate(IOREQ_MAXNUM, sizeof(IoReq_t *));
  DASSERT(fd->ioQueue != NULL);

  xTaskCreate(FloppyReader, "FloppyReader", configMINIMAL_STACK_SIZE, FloppyDev,
              aFloppyIOTaskPrio, &fd->ioTask);
  DASSERT(fd->ioTask != NULL);

  fd->diskTrack = pvPortMallocChip(RAW_TRACK_SIZE);
  fd->track = -1;
  DASSERT(fd->diskTrack != NULL);

  Device_t *dev;
  AddDevice("floppy", &FloppyOps, &dev);
  dev->data = FloppyDev;
  dev->size = FLOPPY_SIZE;
  return dev;
}

static void FloppyMotorOff(FloppyDev_t *fd);

void FloppyKill(void) {
  FloppyDev_t *fd = FloppyDev;

  DisableINT(INTF_DSKBLK);
  DisableDMA(DMAF_DISK);
  ResetIntVec(DSKBLK);
  FloppyMotorOff(fd);

  ReleaseTimer(fd->timer);
  vTaskDelete(fd->ioTask);
  vQueueDelete(fd->ioQueue);
}

/******************************************************************************/

#define READ 0
#define WRITE 1

#define LOWER 0
#define UPPER 1

#define OUTWARDS 0
#define INWARDS 1

#define STEP_SETTLE TIMER_MS(3)

static void StepHeads(FloppyDev_t *fd) {
  BCLR(ciab.ciaprb, CIAB_DSKSTEP);
  BSET(ciab.ciaprb, CIAB_DSKSTEP);

  WaitTimerSleep(fd->timer, STEP_SETTLE);

  fd->headTrk += fd->headDir;
}

#define DIRECTION_REVERSE_SETTLE TIMER_MS(18)

static inline void HeadsStepDirection(FloppyDev_t *fd, int16_t inwards) {
  if (inwards) {
    BCLR(ciab.ciaprb, CIAB_DSKDIREC);
    fd->headDir = 2;
  } else {
    BSET(ciab.ciaprb, CIAB_DSKDIREC);
    fd->headDir = -2;
  }

  WaitTimerSleep(fd->timer, DIRECTION_REVERSE_SETTLE);
}

static inline void ChangeDiskSide(FloppyDev_t *fd, int16_t upper) {
  if (upper) {
    BCLR(ciab.ciaprb, CIAB_DSKSIDE);
    fd->headTrk++;
  } else {
    BSET(ciab.ciaprb, CIAB_DSKSIDE);
    fd->headTrk--;
  }
}

static inline void WaitDiskReady(void) {
  while (ciaa.ciapra & CIAF_DSKRDY)
    continue;
}

static inline int WriteProtected(void) {
  return !(ciaa.ciapra & CIAF_DSKPROT);
}

static inline int HeadsAtTrack0(void) {
  return !(ciaa.ciapra & CIAF_DSKTRACK0);
}

static void FloppyMotorOn(FloppyDev_t *fd) {
  if (fd->motorOn)
    return;

  BSET(ciab.ciaprb, CIAB_DSKSEL0);
  BCLR(ciab.ciaprb, CIAB_DSKMOTOR);
  BCLR(ciab.ciaprb, CIAB_DSKSEL0);

  WaitDiskReady();

  fd->motorOn = 1;
}

static void FloppyMotorOff(FloppyDev_t *fd) {
  if (!fd->motorOn)
    return;

  BSET(ciab.ciaprb, CIAB_DSKSEL0);
  BSET(ciab.ciaprb, CIAB_DSKMOTOR);
  BCLR(ciab.ciaprb, CIAB_DSKSEL0);

  fd->motorOn = 0;
}

static void FloppyHeadToTrack0(FloppyDev_t *fd) {
  FloppyMotorOn(fd);
  HeadsStepDirection(fd, OUTWARDS);
  while (!HeadsAtTrack0())
    StepHeads(fd);
  HeadsStepDirection(fd, INWARDS);
  ChangeDiskSide(fd, LOWER);
  /* Now we are at well defined position */
  fd->headTrk = 0;
}

#define DISK_SETTLE TIMER_MS(15)
#define WRITE_SETTLE TIMER_US(1300)

static int FloppyReadWriteTrack(FloppyDev_t *fd, short cmd, short track) {
  if (cmd == WRITE) {
    if (WriteProtected())
      return EROFS;

    /* Encode sectors that need to be written to disk. */
    for (short i = 0; i < NSECTORS; i++) {
      if (fd->sectorState[i] & DIRTY) {
        EncodeSector(fd->sector[i], fd->diskSector[i]);
        fd->sectorState[i] &= ~DIRTY;
      }
    }

    /* Before a track is written to disk we need to realign it
     * and fix MFM encoding. */
    RealignTrack(fd->diskTrack, fd->diskSector);
  }

  /* Turn the motor on. */
  FloppyMotorOn(fd);

  /* Switch heads if needed. */
  if ((track ^ fd->headTrk) & 1)
    ChangeDiskSide(fd, track & 1);

  /* Travel to requested track. */
  if (track != fd->headTrk) {
    HeadsStepDirection(fd, track > fd->headTrk);
    while (track != fd->headTrk)
      StepHeads(fd);
  }

  /* Wait for the head to stabilize over the track. */
  WaitTimerSleep(fd->timer, DISK_SETTLE);

  /* Make sure the DMA for the disk is turned off. */
  custom.dsklen = 0;

  DPRINTF("[Floppy] %s track %d.\n", (cmd == WRITE) ? "Write" : "Read",
          (int)track);

  uint16_t adkconSet = ADKF_SETCLR | ADKF_MFMPREC | ADKF_FAST;
  uint16_t adkconClr = ADKF_WORDSYNC | ADKF_MSBSYNC;
  if (cmd == READ)
    adkconSet |= ADKF_WORDSYNC;
  if (track < NTRACKS / 2)
    adkconClr |= ADKF_PRECOMP1 | ADKF_PRECOMP0;
  else
    adkconSet |= ADKF_PRECOMP0;
  custom.adkcon = adkconClr;
  custom.adkcon = adkconSet;

  /* Prepare for transfer. */
  ClearIRQ(INTF_DSKBLK);
  EnableINT(INTF_DSKBLK);
  EnableDMA(DMAF_DISK);

  /* Buffer in chip memory. */
  custom.dskpt = fd->diskTrack;

  /* Write track size twice to initiate DMA transfer. */
  uint16_t dsklen = DSK_DMAEN | (RAW_TRACK_SIZE / sizeof(int16_t));
  if (cmd == WRITE)
    dsklen |= DSK_WRITE;
  custom.dsklen = dsklen;
  custom.dsklen = dsklen;

  (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  if (cmd == WRITE)
    WaitTimerSleep(fd->timer, WRITE_SETTLE);

  /* Disable DMA & interrupts. */
  custom.dsklen = 0;
  DisableINT(INTF_DSKBLK);
  DisableDMA(DMAF_DISK);

  if (cmd == READ) {
    fd->track = track;
    memset(fd->sectorState, 0, NSECTORS);
    DecodeTrack(fd->diskTrack, fd->diskSector);
  }

  return 0;
}

static void FloppyReader(void *ptr) {
  FloppyDev_t *fd = ptr;

  FloppyHeadToTrack0(fd);

  for (;;) {
    IoReq_t *io;

    if (!xQueueReceive(fd->ioQueue, &io, 1000 / portTICK_PERIOD_MS)) {
      FloppyMotorOff(fd);
      continue;
    }

    uint8_t needWrite = 0;
    int16_t track = io->offset / TRACK_SIZE;
    int16_t sector = (io->offset / SECTOR_SIZE) % NSECTORS;
    int32_t offset = io->offset % SECTOR_SIZE;

    /* The loop processes one sector at a time. */
    while (io->left > 0) {
      /* if `trackBuf` stores another track or is empty,
       * then we need to read a track */
      if (fd->track != track)
        FloppyReadWriteTrack(fd, READ, track);

      /* let's see if sector we want to read from is decoded */
      if (fd->sectorState[sector] & DECODED) {
        DecodeSector(fd->diskSector[sector], fd->sector[sector]);
        fd->sectorState[sector] |= DECODED;
      }

      size_t n = min(io->left, SECTOR_SIZE - offset);

      if (!io->write) {
        /* copy the data from decoded sector */
        memcpy(io->rbuf, (void *)fd->sector[sector] + offset, n);
        io->rbuf += n;
      } else {
        /* copy the data into decoded sector */
        memcpy((void *)fd->sector[sector] + offset, io->wbuf, n);
        io->wbuf += n;
        needWrite = -1;
      }

      io->left -= n;

      /* Assume we crossed sector boundary, otherwise we quit the loop anyway,
       * and update sector / track counter appropriately. */
      offset = 0;
      if (++sector == NSECTORS) {
        sector = 0;
        if (needWrite) {
          FloppyReadWriteTrack(fd, WRITE, track);
          needWrite = 0;
        }
        track++;
      }
    }

    if (needWrite)
      FloppyReadWriteTrack(fd, WRITE, track);

    io->error = 0;
    IoReqNotify(io);
  }
}

static int FloppyReadWrite(Device_t *dev, IoReq_t *io) {
  FloppyDev_t *fd = dev->data;
  if (io->offset >= (off_t)FLOPPY_SIZE)
    return EINVAL;
  if (io->offset + io->left > FLOPPY_SIZE)
    io->left = FLOPPY_SIZE - io->offset;
  if (io->left == 0)
    return 0;
  xQueueSend(fd->ioQueue, io, portMAX_DELAY);
  IoReqNotifyWait(io, NULL);
  return io->error;
}
