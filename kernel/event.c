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

static EventWaitNote_t *LookupNote(EventWaitList_t *wl, TaskHandle_t listener) {
  EventWaitNote_t *wn = NULL;
  TAILQ_FOREACH (wn, wl, link) {
    if (wn->listener == listener)
      break;
  }
  return wn;
}

int EventMonitor(EventWaitList_t *wl, EvAction_t act) {
  if (act != EV_ADD && act != EV_DELETE)
    return EINVAL;

  TaskHandle_t listener = xTaskGetCurrentTaskHandle();
  EventWaitNote_t *note = NULL;
  int error = 0;

  if (act == EV_ADD)
    note = MemAlloc(sizeof(EventWaitNote_t), MF_ZERO);

  taskENTER_CRITICAL();
  {
    EventWaitNote_t *found = LookupNote(wl, listener);
    if (act == EV_ADD) {
      if (found == NULL) {
        note->listener = listener;
        TAILQ_INSERT_TAIL(wl, note, link);
        note = NULL; /* so it doesn't get freed */
      } else {
        error = EEXIST;
      }
    } else if (act == EV_DELETE) {
      if (found != NULL) {
        TAILQ_REMOVE(wl, found, link);
        note = found; /* we want to free it */
      } else {
        error = ESRCH;
      }
    } else {
      error = EINVAL;
    }
  }
  taskEXIT_CRITICAL();

  if (note)
    MemFree(note);

  return error;
}
