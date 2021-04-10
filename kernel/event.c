#include <event.h>
#include <sys/errno.h>

void EventWaitListInit(EventWaitList_t *wl) {
  (void)wl;
}

void EventNotifyFromISR(EventWaitList_t *wl) {
  (void)wl;
}

int EventMonitor(EventWaitList_t *wl, uint32_t notifyBits) {
  (void)wl, (void)notifyBits;
  return ENOSYS;
}
