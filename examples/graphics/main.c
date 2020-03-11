#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>

#include <palette.h>
#include <bitmap.h>
#include <blitter.h>

#include "data/simpsons-bg.c"
#include "data/bart.c"
#include "data/homer.c"
#include "data/marge.c"
#include "data/screen.c"

static COPLIST(cp, 100);
static copins_t *bplpt[4];

static void vMainTask(__unused void *data) {
  bltcopy_t bc;

  /* Uses double buffering! */
  for (int buffer = 1;; buffer ^= 1) {
    bitmap_t *screen = buffer ? &screen1_bm : &screen0_bm;
    
    BltCopySetSrc(&bc, &simpsons_bm, 0, 0, -1, -1);
    BltCopySetDst(&bc, screen, 0, 0);
    BitmapCopy(&bc);

    /* Calculate position of Bart on screen. */
    short frame = xTaskGetTickCount();
    short anim = (frame >> 2) & 7;
    short x = frame & 255;
    if (frame & 256) {
      x = 255 - x;
      anim += 8;
    } 

    BltCopySetSrc(&bc, &bart_bm, 16 * anim, 0, 16, -1);
    BltCopySetDst(&bc, screen, 32 + x, 132);
    BitmapCopy(&bc);

    WaitBlitter();

    /* Wait for lower part of raster that is not visible. */
    WaitLine(VP(176 + 40));

    /* Swap bitplanes for those that we won't write to during next frame. */
    for (int i = 0; i < screen->depth; i++)
      CopInsSet32(bplpt[i], screen->planes[i]);
  }
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

static xTaskHandle main_handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  AddIntServer(VertBlankChain, SystemClockTick);

  CopSetupScreen(cp, &screen0_bm, MODE_LORES, HP(0), VP(40));
  CopSetupBitplanes(cp, &screen0_bm, bplpt);
  CopLoadPal(cp, &simpsons_pal, 0);
  CopEnd(cp);

  CopListActivate(cp);
  EnableDMA(DMAF_RASTER|DMAF_BLITTER);

  xTaskCreate(vMainTask, "main", configMINIMAL_STACK_SIZE, NULL, 0,
              &main_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {}
