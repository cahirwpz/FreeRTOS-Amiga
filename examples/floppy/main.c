#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <floppy.h>
#include <serial.h>
#include <libkern.h>
#include <interrupt.h>

extern void vReaderTask(void);
extern void vWriterTask(void);

static void vPlusTask(File_t *ser) {
  for (;;)
    kfputchar(ser, '-');
}

static void vMinusTask(File_t *ser) {
  for (;;)
    kfputchar(ser, '+');
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

  File_t *ser = SerialOpen(9600);

  FloppyInit(FLOPPY_TASK_PRIO);

  xTaskCreate((TaskFunction_t)vPlusTask, "plus", configMINIMAL_STACK_SIZE, ser,
              PLUS_TASK_PRIO, &plusHandle);
  xTaskCreate((TaskFunction_t)vMinusTask, "minus", configMINIMAL_STACK_SIZE,
              ser, MINUS_TASK_PRIO, &minusHandle);
  xTaskCreate((TaskFunction_t)vReaderTask, "reader", configMINIMAL_STACK_SIZE,
              NULL, READER_TASK_PRIO, &readerHandle);
  xTaskCreate((TaskFunction_t)vWriterTask, "writer", configMINIMAL_STACK_SIZE,
              NULL, WRITER_TASK_PRIO, &writerHandle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
  custom.color[0] = 0;
}
