#pragma once

#include <sys/queue.h>
#include <sys/types.h>

typedef struct tskTaskControlBlock *TaskHandle_t;
typedef struct EventWaitNote EventWaitNote_t;

typedef enum EvAction {
  EV_ADD,    /* add event reporting */
  EV_DELETE, /* remove event reporting */
} __packed EvAction_t;

/* Kind of events a task can wait for. */
typedef enum EvFilter {
  EVFILT_READ,  /* can-read: there's new data to be read */
  EVFILT_WRITE, /* can-write there's more space available for write */
} __packed EvFilter_t;

/* List of tasks waiting for an event to happen.
 * Must be initialized with TAILQ_INIT before use! */
typedef TAILQ_HEAD(, EventWaitNote) EventWaitList_t;

/* Called from ISR to wake up tasks waiting for given event. */
void EventNotifyFromISR(EventWaitList_t *wl);

/* Add or remove calling task to the list of event listeners. */
int EventMonitor(EventWaitList_t *wl, EvAction_t act);
