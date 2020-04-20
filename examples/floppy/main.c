#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/queue.h>

#include <interrupt.h>
#include <stdio.h>

#include <floppy.h>
#include <serial.h>

#define mainPLUS_TASK_PRIORITY 2
#define mainMINUS_TASK_PRIORITY 2
#define mainREADER_TASK_PRIORITY 3

static File_t *ser = NULL;

static void vPlusTask(__unused void *data) {
  for (;;) {
    FilePutChar(ser, '-');
  }
}

static void vMinusTask(__unused void *data) {
  for (;;) {
    FilePutChar(ser, '+');
  }
}

static void SendIO(FloppyIO_t *io, short track) {
  io->track = track;
  FloppySendIO(io);
}

static void WaitIO(QueueHandle_t replyQ, void *buf) {
  FloppyIO_t *io = NULL;
  (void)xQueueReceive(replyQ, &io, portMAX_DELAY);
  DecodeTrack(io->buffer, buf);
}

static void vReaderTask(__unused void *data) {
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

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

static xTaskHandle plus_handle;
static xTaskHandle minus_handle;
static xTaskHandle reader_handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  AddIntServer(VertBlankChain, SystemClockTick);

  FloppyInit(mainREADER_TASK_PRIORITY);

  ser = SerialOpen(9600);

  xTaskCreate(vPlusTask, "plus", configMINIMAL_STACK_SIZE, NULL,
              mainPLUS_TASK_PRIORITY, &plus_handle);

  xTaskCreate(vMinusTask, "minus", configMINIMAL_STACK_SIZE, NULL,
              mainMINUS_TASK_PRIORITY, &minus_handle);

  xTaskCreate(vReaderTask, "reader", configMINIMAL_STACK_SIZE, NULL,
              mainREADER_TASK_PRIORITY, &reader_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) { custom.color[0] = 0; }
