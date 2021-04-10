#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <console.h>
#include <device.h>
#include <file.h>
#include <input.h>
#include <interrupt.h>
#include <ioreq.h>
#include <keyboard.h>
#include <libkern.h>
#include <mouse.h>

#define mainINPUT_TASK_PRIORITY 3

static const char *EventName[] = {
  [IE_UNKNOWN] = "unknown",
  [IE_MOUSE_UP] = "mouse button up",
  [IE_MOUSE_DOWN] = "mouse button down",
  [IE_MOUSE_DELTA_X] = "mouse delta X",
  [IE_MOUSE_DELTA_Y] = "mouse delta Y",
  [IE_KEYBOARD_UP] = "keyboard key up",
  [IE_KEYBOARD_DOWN] = "keyboard key down",
};

#define MOUSE BIT(0)
#define KEYBOARD BIT(1)

static void vInputTask(void *data) {
  File_t *cons = data;
  Device_t *ms = MouseInit();
  Device_t *kbd = KeyboardInit();

  IoReq_t kbdIo, msIo;
  InputEvent_t ev;
  uint32_t value = 0;

  do {
    int kbdErr, msErr;

    do {
      kbdIo = IOREQ_READ(0, &ev, sizeof(InputEvent_t));
      kbdIo.async = 1;
      kbdIo.notifyBits = KEYBOARD;

      if (!(kbdErr = kbd->ops->read(kbd, &kbdIo))) {
        char c = (ev.value >= 0x20 && ev.value < 0x7f) ? ev.value : ' ';
        kfprintf(cons, "%s: value = %x, char = '%c'\n", EventName[ev.kind],
                 (uint16_t)ev.value, c);
      }

      msIo = IOREQ_READ(0, &ev, sizeof(InputEvent_t));
      msIo.async = 1;
      msIo.notifyBits = KEYBOARD;

      if (!(msErr = ms->ops->read(ms, &msIo))) {
        static short x = 0, y = 0;
        kfprintf(cons, "%s: value = %d\n", EventName[ev.kind], ev.value);
        if (ev.kind == IE_MOUSE_DELTA_X) {
          x += ev.value;
          x = max(0, x);
          x = min(x, 319);
        }
        if (ev.kind == IE_MOUSE_DELTA_Y) {
          y += ev.value;
          y = max(0, y);
          y = min(y, 255);
        }
        ConsoleMovePointer(x, y);
      }
    } while (!kbdErr || !msErr);
  } while (xTaskNotifyWait(0, MOUSE | KEYBOARD, &value, portMAX_DELAY));
}

static void SystemClockTickHandler(__unused void *data) {
  /* Increment the system timer value and possibly preempt. */
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();
  xNeedRescheduleTask = xTaskIncrementTick();
  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

INTSERVER_DEFINE(SystemClockTick, 10, SystemClockTickHandler, NULL);

static xTaskHandle input_handle;

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  /* Configure system clock. */
  AddIntServer(VertBlankChain, SystemClockTick);

  (void)ConsoleInit();

  File_t *cons = kopen("console", O_RDWR);

  xTaskCreate(vInputTask, "input", configMINIMAL_STACK_SIZE, cons,
              mainINPUT_TASK_PRIORITY, &input_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
}
