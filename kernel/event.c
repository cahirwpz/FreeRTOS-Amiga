#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <event.h>
#include <memory.h>
#include <notify.h>
#include <sys/errno.h>

struct EventWaitNote {
  TAILQ_ENTRY(EventWaitNote) link;
  TaskHandle_t listener;
};

void EventNotifyFromISR(EventWaitList_t *wl) {
  EventWaitNote_t *wn;
  TAILQ_FOREACH (wn, wl, link) { NotifySendFromISR(wn->listener, NB_EVENT); }
}

int EventMonitor(EventWaitList_t *wl) {
  TaskHandle_t listener = xTaskGetCurrentTaskHandle();
  EventWaitNote_t *note = MemAlloc(sizeof(EventWaitNote_t), MF_ZERO);
  int error = 0;

  taskENTER_CRITICAL();
  {
    EventWaitNote_t *wn;
    TAILQ_FOREACH (wn, wl, link) {
      if (wn->listener == listener) {
        error = EEXIST;
        break;
      }
    }
    if (!error) {
      note->listener = listener;
      TAILQ_INSERT_TAIL(wl, note, link);
    }
  }
  taskEXIT_CRITICAL();

  if (error)
    MemFree(note);

  return error;
}
