#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <custom.h>
#include <trap.h>

extern int rand(void);

static void vMainTask(__unused void *data) {
  for (;;) {
    custom.color[0] = rand();
  }
}

static xTaskHandle handle;

extern void vPortDefaultTrapHandler(struct TrapFrame *);

/* TODO: use C bit fields to define instruction encoding */
typedef struct MulsInst {
} MulsInst_t;

typedef struct DivsInst {
} DivsInst_t;

/* Keep in mind that:
 * - PC & SR are placed at different positions in the trap frame
 *   for 68000 and 68010 processors
 * - gcc replaces 32-bit multiplication and division by calls to
 *   __mulsi3 and __divsi3 procedures
 */
static bool EmulMuls(struct TrapFrame *frame) {
  /* TODO: implement muls.l instruction emulation */
  (void)frame;
  return false;
}

static bool EmulDivs(struct TrapFrame *frame) {
  /* TODO: implement divsl.l instruction emulation */
  (void)frame;
  return false;
}

void vPortTrapHandler(struct TrapFrame *frame) {
  if (EmulMuls(frame) || EmulDivs(frame))
    return;

  custom.color[0] = 0xf00;
  vPortDefaultTrapHandler(frame);
}

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  xTaskCreate(vMainTask, "main", configMINIMAL_STACK_SIZE, NULL, 0, &handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) { custom.color[0] = 0x00f; }
