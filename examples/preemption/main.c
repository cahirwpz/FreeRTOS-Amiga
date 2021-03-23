#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <libkern.h>
#include <stdlib.h>

#define RED_TASK_PRIORITY 2
#define GREEN_TASK_PRIORITY 2
#define TESTHEAP_TASK_PRIORITY 3

#define NSLOTS 400
#define MAXBLKSZ 4096

static void *Slot[NSLOTS];
static int FreeSlot = 0; /* points to first free slot */

static int rand(void) {
  static unsigned seed = 0xDEADC0DE;
  return rand_r(&seed);
}

static void RandMalloc(void) {
  if (FreeSlot == NSLOTS)
    return;

  int sz = 1 + (rand() % MAXBLKSZ);
  void *p = kmalloc(sz);
  klog("malloc(%d) = %x\n", sz, p);
  Slot[FreeSlot++] = p;
}

static void RandFree(void) {
  if (FreeSlot == 0)
    return;

  int n = rand() % FreeSlot;
  void *p = Slot[n];
  kfree(p);
  klog("free(%p)\n", p);
  Slot[n] = NULL;

  int last = --FreeSlot;
  if (n != last)
    Slot[n] = Slot[last];
}

static void RandRealloc(void) {
  if (FreeSlot == 0)
    return;

  int n = rand() % FreeSlot;
  void *p = Slot[n];
  int sz = 1 + (rand() % MAXBLKSZ);
  void *q = krealloc(p, sz);
  klog("realloc(%p, %d) = %p\n", p, sz, q);
  Slot[n] = q;
}

static void vTestHeapTask(__unused void *data) {
  for (short i = 0; i < NSLOTS * 3 / 4; i++)
    RandMalloc();
  kmcheck(0);

  for (;;) {
    int c = rand() % 4;
    if (c == 0)
      RandMalloc();
    else if (c == 1)
      RandFree();
    else
      RandRealloc();
    kmcheck(0);
    vTaskDelay(1);
  }
}

static void vRedTask(__unused void *data) {
  for (;;) {
    custom.color[0] = 0xf00;
  }
}

static void vGreenTask(__unused void *data) {
  for (;;) {
    custom.color[0] = 0x0f0;
  }
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

static xTaskHandle redHandle;
static xTaskHandle greenHandle;
static xTaskHandle testHeapHandle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  AddIntServer(VertBlankChain, SystemClockTick);

  xTaskCreate(vRedTask, "red", configMINIMAL_STACK_SIZE, NULL,
              RED_TASK_PRIORITY, &redHandle);

  xTaskCreate(vGreenTask, "green", configMINIMAL_STACK_SIZE, NULL,
              GREEN_TASK_PRIORITY, &greenHandle);

  xTaskCreate(vTestHeapTask, "test-heap", configMINIMAL_STACK_SIZE, NULL,
              TESTHEAP_TASK_PRIORITY, &testHeapHandle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
  custom.color[0] = 0x00f;
}
