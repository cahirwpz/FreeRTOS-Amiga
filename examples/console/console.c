#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <driver.h>
#include <devfile.h>
#include <file.h>
#include <event.h>
#include <input.h>
#include <display.h>
#include <ioreq.h>
#include <notify.h>

#define INPUT_TASK_PRIO 2

static const char *EventName[] = {
  [IE_UNKNOWN] = "unknown",
  [IE_MOUSE_UP] = "mouse button up",
  [IE_MOUSE_DOWN] = "mouse button down",
  [IE_MOUSE_DELTA_X] = "mouse delta X",
  [IE_MOUSE_DELTA_Y] = "mouse delta Y",
  [IE_KEYBOARD_UP] = "keyboard key up",
  [IE_KEYBOARD_DOWN] = "keyboard key down",
};

void vConsoleTask(void *data __unused) {
  File_t *disp, *ms, *kbd;

  FileOpen("display", O_WRONLY, &disp);
  FileOpen("mouse", O_RDONLY | O_NONBLOCK, &ms);
  FileOpen("keyboard", O_RDONLY | O_NONBLOCK, &kbd);

  (void)FileEvent(ms, EV_ADD, EVFILT_READ);
  (void)FileEvent(kbd, EV_ADD, EVFILT_READ);

  while (NotifyWait(NB_EVENT, portMAX_DELAY)) {
    InputEvent_t ev;

    while (!FileRead(kbd, &ev, sizeof(ev), NULL)) {
      char c = (ev.value >= 0x20 && ev.value < 0x7f) ? ev.value : ' ';
      FilePrintf(disp, "%s: value = %x, char = '%c'\n", EventName[ev.kind],
                 (uint16_t)ev.value, c);
    }

    while (!FileRead(ms, &ev, sizeof(ev), NULL)) {
      static MousePos_t m = {.x = 0, .y = 0};

      FilePrintf(disp, "%s: value = %d\n", EventName[ev.kind], ev.value);

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

      FileIoctl(disp, DIOCSETMS, &m);
    }
  }
}
