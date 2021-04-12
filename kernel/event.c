#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <event.h>
#include <libkern.h>
#include <notify.h>
#include <sys/errno.h>

void EventWaitListInit(EventWaitList_t *wl) {
  TAILQ_INIT(wl);
}

void EventNotifyFromISR(EventWaitList_t *wl) {
  EventWaitNote_t *wn;
  TAILQ_FOREACH (wn, wl, link) {
    NotifySendFromISR(wn->listener, wn->notifyBits);
  }
}

int EventMonitor(EventWaitList_t *wl, uint32_t notifyBits) {
  TaskHandle_t listener = xTaskGetCurrentTaskHandle();
  EventWaitNote_t *wn = kcalloc(1, sizeof(EventWaitNote_t));
  int error = 0;

  taskENTER_CRITICAL();
  TAILQ_FOREACH (wn, wl, link) {
    if (wn->listener == listener && wn->notifyBits == notifyBits) {
      error = EEXIST;
      break;
    }
  }
  if (!error)
    TAILQ_INSERT_TAIL(wl, wn, link);
  taskEXIT_CRITICAL();

  return error;
}
