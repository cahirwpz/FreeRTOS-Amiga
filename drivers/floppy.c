#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/queue.h>

#include <interrupt.h>
#include <custom.h>
#include <cia.h>

#include <stdint.h>
#include <libkern.h>

#include <floppy.h>

#define DEBUG 0

#define FLOPPYIO_MAXNUM 8

static CIATimer_t *FloppyTimer;
static xTaskHandle FloppyIOTask;
static QueueHandle_t FloppyIOQueue;

static void TrackTransferDone(__unused void *ptr) {
  /* Send notification to waiting task. */
  vTaskNotifyGiveFromISR(FloppyIOTask, &xNeedRescheduleTask);
}

static void FloppyReader(void *);

void FloppyInit(unsigned aFloppyIOTaskPrio) {
  kprintf("[Init] Floppy drive driver!\n");

  FloppyTimer = AcquireTimer(TIMER_ANY);
  configASSERT(FloppyTimer != NULL);

  /* Set standard synchronization marker. */
  custom.dsksync = DSK_SYNC;

  /* Standard settings for Amiga format disk floppies. */
  custom.adkcon = ADKF_SETCLR | ADKF_MFMPREC | ADKF_WORDSYNC | ADKF_FAST;

  /* Handler that will wake up track reader task. */
  SetIntVec(DSKBLK, TrackTransferDone, NULL);

  FloppyIOQueue = xQueueCreate(FLOPPYIO_MAXNUM, sizeof(FloppyIO_t *));
  configASSERT(FloppyIOQueue != NULL);

  xTaskCreate(FloppyReader, "FloppyReader", configMINIMAL_STACK_SIZE, NULL,
              aFloppyIOTaskPrio, &FloppyIOTask);
  configASSERT(FloppyIOTask != NULL);
}

static void FloppyMotorOff(void);

void FloppyKill(void) {
  DisableINT(INTF_DSKBLK);
  DisableDMA(DMAF_DISK);
  ResetIntVec(DSKBLK);
  FloppyMotorOff();

  ReleaseTimer(FloppyTimer);
  vTaskDelete(FloppyIOTask);
  vQueueDelete(FloppyIOQueue);
}

/******************************************************************************/

#define LOWER 0
#define UPPER 1

#define OUTWARDS 0
#define INWARDS 1

static int16_t MotorOn;
static int16_t HeadDir;
static int16_t Track;

#define STEP_SETTLE TIMER_MS(3)

static void StepHeads(void) {
  BCLR(ciab.ciaprb, CIAB_DSKSTEP);
  BSET(ciab.ciaprb, CIAB_DSKSTEP);

  WaitTimerSleep(FloppyTimer, STEP_SETTLE);

  Track += HeadDir;
}

#define DIRECTION_REVERSE_SETTLE TIMER_MS(18)

static inline void HeadsStepDirection(int16_t inwards) {
  if (inwards) {
    BCLR(ciab.ciaprb, CIAB_DSKDIREC);
    HeadDir = 2;
  } else {
    BSET(ciab.ciaprb, CIAB_DSKDIREC);
    HeadDir = -2;
  }

  WaitTimerSleep(FloppyTimer, DIRECTION_REVERSE_SETTLE);
}

static inline void ChangeDiskSide(int16_t upper) {
  if (upper) {
    BCLR(ciab.ciaprb, CIAB_DSKSIDE);
    Track++;
  } else {
    BSET(ciab.ciaprb, CIAB_DSKSIDE);
    Track--;
  }
}

static inline void WaitDiskReady(void) {
  while (ciaa.ciapra & CIAF_DSKRDY)
    continue;
}

static inline int WriteProtected(void) {
  return !(ciaa.ciapra & CIAF_DSKPROT);
}

static inline int HeadsAtTrack0() {
  return !(ciaa.ciapra & CIAF_DSKTRACK0);
}

static void FloppyMotorOn(void) {
  if (MotorOn)
    return;

  BSET(ciab.ciaprb, CIAB_DSKSEL0);
  BCLR(ciab.ciaprb, CIAB_DSKMOTOR);
  BCLR(ciab.ciaprb, CIAB_DSKSEL0);

  WaitDiskReady();

  MotorOn = 1;
}

static void FloppyMotorOff(void) {
  if (!MotorOn)
    return;

  BSET(ciab.ciaprb, CIAB_DSKSEL0);
  BSET(ciab.ciaprb, CIAB_DSKMOTOR);
  BCLR(ciab.ciaprb, CIAB_DSKSEL0);

  MotorOn = 0;
}

#define DISK_SETTLE TIMER_MS(15)
#define WRITE_SETTLE TIMER_US(1300)

static void FloppyReader(__unused void *ptr) {
  /* Move head to track 0 */
  FloppyMotorOn();
  HeadsStepDirection(OUTWARDS);
  while (!HeadsAtTrack0())
    StepHeads();
  HeadsStepDirection(INWARDS);
  ChangeDiskSide(LOWER);
  /* Now we are at well defined position */
  Track = 0;

  for (;;) {
    FloppyIO_t *io;

    if (xQueueReceive(FloppyIOQueue, &io, 1000 / portTICK_PERIOD_MS)) {
      if (io->cmd == CMD_WRITE)
        configASSERT(!WriteProtected());

      /* Turn the motor on. */
      FloppyMotorOn();

      /* Switch heads if needed. */
      if ((io->track ^ Track) & 1)
        ChangeDiskSide(io->track & 1);

      /* Travel to requested track. */
      if (io->track != Track) {
        HeadsStepDirection(io->track > Track);
        while (io->track != Track)
          StepHeads();
      }

      /* Wait for the head to stabilize over the track. */
      WaitTimerSleep(FloppyTimer, DISK_SETTLE);

      /* Make sure the DMA for the disk is turned off. */
      custom.dsklen = 0;

#if DEBUG
      printf("[Floppy] %s track %d into %p.\n",
             (io->cmd == CMD_WRITE) ? "Write" : "Read", (int)io->track,
             io->buffer);
#endif

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
        WaitTimerSleep(FloppyTimer, WRITE_SETTLE);

      /* Disable DMA & interrupts. */
      custom.dsklen = 0;
      DisableINT(INTF_DSKBLK);
      DisableDMA(DMAF_DISK);

      /* Wake up the task that requested transfer. */
      xQueueSend(io->replyQueue, &io, portMAX_DELAY);
    } else {
      FloppyMotorOff();
    }
  }
}

void FloppySendIO(FloppyIO_t *io) {
  configASSERT(io->track < TRACK_COUNT);
  configASSERT(io->replyQueue != NULL);
  configASSERT(io->buffer != NULL);

  xQueueSend(FloppyIOQueue, &io, portMAX_DELAY);
}
