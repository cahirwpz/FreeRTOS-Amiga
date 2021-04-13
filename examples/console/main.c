#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <driver.h>
#include <devfile.h>
#include <file.h>
#include <event.h>
#include <input.h>
#include <console.h>
#include <interrupt.h>
#include <ioreq.h>
#include <notify.h>
#include <libkern.h>

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

static void vInputTask(void *data __unused) {
  File_t *cons = kopen("console", O_WRONLY);
  File_t *ms = kopen("mouse", O_RDONLY | O_NONBLOCK);
  File_t *kbd = kopen("keyboard", O_RDONLY | O_NONBLOCK);

  (void)FileEvent(ms, EV_READ);
  (void)FileEvent(kbd, EV_READ);

  while (NotifyWait(NB_EVENT, portMAX_DELAY)) {
    InputEvent_t ev;
    long done;

    while (!FileRead(kbd, &ev, sizeof(ev), &done)) {
      char c = (ev.value >= 0x20 && ev.value < 0x7f) ? ev.value : ' ';
      kfprintf(cons, "%s: value = %x, char = '%c'\n", EventName[ev.kind],
               (uint16_t)ev.value, c);
    }

    while (!FileRead(ms, &ev, sizeof(ev), &done)) {
      static MousePos_t m = {.x = 0, .y = 0};

      kfprintf(cons, "%s: value = %d\n", EventName[ev.kind], ev.value);

      if (ev.kind == IE_MOUSE_DELTA_X) {
        m.x += ev.value;
        m.x = max(0, m.x);
        m.x = min(m.x, 319);
      }

      if (ev.kind == IE_MOUSE_DELTA_Y) {
        m.y += ev.value;
        m.y = max(0, m.y);
        m.y = min(m.y, 255);
      }

      FileIoctl(cons, CIOCSETMS, &m);
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

int main(void) {
  portNOP(); /* Breakpoint for simulator. */

  /* Configure system clock. */
  AddIntServer(VertBlankChain, SystemClockTick);

  DeviceAttach(&Console);
  DeviceAttach(&Mouse);
  DeviceAttach(&Keyboard);

  xTaskCreate(vInputTask, "input", configMINIMAL_STACK_SIZE, NULL,
              mainINPUT_TASK_PRIORITY, &input_handle);

  vTaskStartScheduler();

  return 0;
}

void vApplicationIdleHook(void) {
}
