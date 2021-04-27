#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <cpu.h>
#include <custom.h>
#include <trap.h>

extern int rand(void);

static void vMainTask(__unused void *data) {
  for (;;) {
    custom.color[0] = rand();
  }
}

static xTaskHandle handle;

extern void vPortDefaultTrapHandler(TrapFrame_t *);

/* TODO: use C bit fields to define instruction encodings
 * which are given on pages 568-569 of M68000PRM */
typedef struct MulsInst {
} MulsInst_t;

typedef struct DivsInst {
} DivsInst_t;

/* These functions are part of libc library (`gen` subdirectory)
 * and emulate full 32-bit multiplication and division on M68000. */
extern int32_t __mulsi3(int32_t a, int32_t b);
extern int32_t __divsi3(int32_t a, int32_t b);
extern int32_t __modsi3(int32_t a, int32_t b);
extern uint32_t __udivsi3(uint32_t a, uint32_t b);
extern uint32_t __umodsi3(uint32_t a, uint32_t b);

/* You can invoke assembly inlines from <cpu.h> to force compiler to use
 * 16 x 16 -> 32 multiplication or 32 / 16 -> 16 division instructions
 * available in M68000. */

/* Keep in mind that:
 * - PC & SR are placed at different positions in the trap frame
 *   for 68000 and 68010 processors
 * - gcc replaces 32-bit multiplication and division in code
 *   by calls to __mulsi3 and __divsi3 procedures on 68000 and 68010
 */
static bool EmulMuls(TrapFrame_t *frame) {
  /* TODO: implement muls.l instruction emulation */
  (void)frame;
  return false;
}

static bool EmulDivs(TrapFrame_t *frame) {
  /* TODO: implement divsl.l instruction emulation */
  (void)frame;
  return false;
}

void vPortTrapHandler(TrapFrame_t *frame) {
  if (EmulMuls(frame) || EmulDivs(frame))
    return;

  custom.color[0] = 0xf00;
  vPortDefaultTrapHandler(frame);
}

int main(void) {
  NOP(); /* Breakpoint for simulator. */

  xTaskCreate(vMainTask, "main", configMINIMAL_STACK_SIZE, NULL, 0, &handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
  custom.color[0] = 0x00f;
}
