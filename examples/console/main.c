#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <interrupt.h>
#include <stdio.h>

#include <event.h>
#include <copper.h>
#include <palette.h>
#include <bitmap.h>
#include <sprite.h>
#include <font.h>

#include "console.h"
#include "data/lat2-08.c"
#include "data/pointer.c"

#define mainINPUT_TASK_PRIORITY 3

static void vInputTask(__unused void *data) {
  for (;;) {
    Event_t ev;
    if (!PopEvent(&ev))
      continue;
    if (ev.type == EV_MOUSE) {
      ConsolePrintf("MOUSE: x = %d, y = %d, button = %x\n",
                    ev.mouse.x, ev.mouse.y, ev.mouse.button);
      SpriteUpdatePos(&pointer_spr, HP(ev.mouse.x), VP(ev.mouse.y));
    } else if (ev.type == EV_KEY) {
      ConsolePrintf("KEY: ascii = '%c', code = %x, modifier = %x\n",
                    ev.key.ascii, ev.key.code, ev.key.modifier);
    }
  }
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

static xTaskHandle input_handle;

#include "data/screen.c"

static COPLIST(cp, 40);

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  /* Configure system clock. */
  AddIntServer(VertBlankChain, SystemClockTick);

  /*
   * Copper configures hardware each frame (50Hz in PAL) to:
   *  - set video mode to HIRES (640x256),
   *  - display one bitplane,
   *  - set background color to black, and foreground to white,
   *  - set up mouse pointer palette,
   *  - set sprite 0 to mouse pointer graphics,
   *  - set other sprites to empty graphics,
   */
  CopSetupScreen(cp, &screen_bm, MODE_HIRES, HP(0), VP(0));
  CopSetupBitplanes(cp, &screen_bm, NULL);
  CopLoadColor(cp, 0, 0x000);
  CopLoadColor(cp, 1, 0xfff);
  CopLoadPal(cp, &pointer_pal, 16);
  CopLoadSprite(cp, 0, &pointer_spr);
  for (int i = 1; i < 8; i++)
    CopLoadSprite(cp, i, NULL);
  CopEnd(cp);

  /* Set sprite position in upper-left corner of display window. */
  SpriteUpdatePos(&pointer_spr, HP(0), VP(0));

  /* Tell copper where the copper list begins and enable copper DMA. */
  CopListActivate(cp);

  /* Enable bitplane and sprite fetchers' DMA. */
  EnableDMA(DMAF_RASTER|DMAF_SPRITE);

  EventQueueInit();
  MouseInit(0, 0, screen_bm.width - 1, screen_bm.height - 1);
  KeyboardInit();

  ConsoleInit(&screen_bm, &console_font);

  xTaskCreate(vInputTask, "input", configMINIMAL_STACK_SIZE, NULL,
              mainINPUT_TASK_PRIORITY, &input_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {}
