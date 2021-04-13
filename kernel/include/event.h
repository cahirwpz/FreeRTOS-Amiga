#pragma once

#include <sys/queue.h>
#include <sys/types.h>

typedef struct tskTaskControlBlock *TaskHandle_t;
typedef struct EventWaitNote EventWaitNote_t;

/* Kind of events a task can wait for. */
typedef enum EvKind {
  EV_READ,  /* can-read: there's new data to be read */
  EV_WRITE, /* can-write there's more space available for write */
} __packed EvKind_t;

/* List of tasks waiting for an event to happen.
 * Must be initialized with TAILQ_INIT before use! */
typedef TAILQ_HEAD(, EventWaitNote) EventWaitList_t;

/* Called from ISR to wake up tasks waiting for given event. */
void EventNotifyFromISR(EventWaitList_t *wl);

/* Add calling task to the list of event listeners. */
int EventMonitor(EventWaitList_t *wl);
