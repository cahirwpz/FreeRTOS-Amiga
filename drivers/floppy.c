#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/queue.h>

#include <interrupt.h>
#include <custom.h>
#include <cia.h>

#include <stdint.h>
#include <libkern.h>
#include <device.h>
#include <ioreq.h>
#include <sys/errno.h>

#include <floppy.h>

#define DEBUG 0
#include <debug.h>

#define FLOPPYIO_MAXNUM 8

static int FloppyRead(Device_t *, IoReq_t *);
static int FloppyWrite(Device_t *, IoReq_t *);

static DeviceOps_t FloppyOps = {.read = FloppyRead, .write = FloppyWrite};

typedef struct FloppyDev {
  CIATimer_t *timer;
  xTaskHandle ioTask;
  QueueHandle_t ioQueue;

  int16_t motorOn;
  int16_t headDir;
  int16_t track;
} FloppyDev_t;

static FloppyDev_t FloppyDev[1];

static void TrackTransferDone(void *ptr) {
  FloppyDev_t *fd = ptr;
  /* Send notification to waiting task. */
  vTaskNotifyGiveFromISR(fd->ioTask, &xNeedRescheduleTask);
}

static void FloppyReader(void *);

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

  fd->ioQueue = xQueueCreate(FLOPPYIO_MAXNUM, sizeof(FloppyIO_t *));
  DASSERT(fd->ioQueue != NULL);

  xTaskCreate(FloppyReader, "FloppyReader", configMINIMAL_STACK_SIZE, FloppyDev,
              aFloppyIOTaskPrio, &fd->ioTask);
  DASSERT(fd->ioTask != NULL);

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

#define LOWER 0
#define UPPER 1

#define OUTWARDS 0
#define INWARDS 1

#define STEP_SETTLE TIMER_MS(3)

static void StepHeads(FloppyDev_t *fd) {
  BCLR(ciab.ciaprb, CIAB_DSKSTEP);
  BSET(ciab.ciaprb, CIAB_DSKSTEP);

  WaitTimerSleep(fd->timer, STEP_SETTLE);

  fd->track += fd->headDir;
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
    fd->track++;
  } else {
    BSET(ciab.ciaprb, CIAB_DSKSIDE);
    fd->track--;
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

#define DISK_SETTLE TIMER_MS(15)
#define WRITE_SETTLE TIMER_US(1300)

static int FloppyReadWriteTrack(FloppyDev_t *fd, FloppyIO_t *io) {
  if (io->cmd == CMD_WRITE && WriteProtected())
    return EROFS;

  /* Turn the motor on. */
  FloppyMotorOn(fd);

  /* Switch heads if needed. */
  if ((io->track ^ fd->track) & 1)
    ChangeDiskSide(fd, io->track & 1);

  /* Travel to requested track. */
  if (io->track != fd->track) {
    HeadsStepDirection(fd, io->track > fd->track);
    while (io->track != fd->track)
      StepHeads(fd);
  }

  /* Wait for the head to stabilize over the track. */
  WaitTimerSleep(fd->timer, DISK_SETTLE);

  /* Make sure the DMA for the disk is turned off. */
  custom.dsklen = 0;

  DPRINTF("[Floppy] %s track %d into %p.\n",
          (io->cmd == CMD_WRITE) ? "Write" : "Read", (int)io->track,
          io->buffer);

  uint16_t adkconSet = ADKF_SETCLR | ADKF_MFMPREC | ADKF_FAST;
  uint16_t adkconClr = ADKF_WORDSYNC | ADKF_MSBSYNC;
  if (io->cmd == CMD_READ)
    adkconSet |= ADKF_WORDSYNC;
  if (io->track < TRACK_COUNT / 2)
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
  custom.dskpt = io->buffer;

  /* Write track size twice to initiate DMA transfer. */
  uint16_t dsklen = DSK_DMAEN | (TRACK_SIZE / sizeof(int16_t));
  if (io->cmd == CMD_WRITE)
    dsklen |= DSK_WRITE;
  custom.dsklen = dsklen;
  custom.dsklen = dsklen;

  (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

  if (io->cmd == CMD_WRITE)
    WaitTimerSleep(fd->timer, WRITE_SETTLE);

  /* Disable DMA & interrupts. */
  custom.dsklen = 0;
  DisableINT(INTF_DSKBLK);
  DisableDMA(DMAF_DISK);

  /* Wake up the task that requested transfer. */
  xQueueSend(io->replyQueue, &io, portMAX_DELAY);

  return 0;
}

static void FloppyReader(void *ptr) {
  FloppyDev_t *fd = ptr;

  /* Move head to track 0 */
  FloppyMotorOn(fd);
  HeadsStepDirection(fd, OUTWARDS);
  while (!HeadsAtTrack0())
    StepHeads(fd);
  HeadsStepDirection(fd, INWARDS);
  ChangeDiskSide(fd, LOWER);
  /* Now we are at well defined position */
  fd->track = 0;

  for (;;) {
    FloppyIO_t *io;

    if (xQueueReceive(fd->ioQueue, &io, 1000 / portTICK_PERIOD_MS)) {
      FloppyReadWriteTrack(fd, io);
    } else {
      FloppyMotorOff(fd);
    }
  }
}

void FloppySendIO(FloppyIO_t *io) {
  FloppyDev_t *fd = FloppyDev;

  DASSERT(io->track < TRACK_COUNT);
  DASSERT(io->replyQueue != NULL);
  DASSERT(io->buffer != NULL);

  xQueueSend(fd->ioQueue, &io, portMAX_DELAY);
}

static int FloppyRead(Device_t *dev, IoReq_t *req) {
  (void)dev, (void)req;
  return 0;
}

static int FloppyWrite(Device_t *dev, IoReq_t *req) {
  (void)dev, (void)req;
  return 0;
}
