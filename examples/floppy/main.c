#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

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

static void vReaderTask(__unused void *data) {
  void *track = AllocFloppyTrack();
  void *buf = pvPortMalloc(512);
  for (;;) {
    DiskTrack_t sectors;
    for (int i = 0; i < 160; i++) {
       ReadFloppyTrack(track, i);
       ParseTrack(track, sectors);
       for (int j = 0; j < TRACK_NSECTORS; j++) {
         DecodeSector(sectors[j], buf);
       }
    }
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
