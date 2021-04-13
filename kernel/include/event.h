#pragma once

#include <sys/queue.h>
#include <sys/types.h>

typedef struct tskTaskControlBlock *TaskHandle_t;

typedef enum EvKind {
  EV_READ,  /* there's new data to read */
  EV_WRITE, /* there's more space available for write */
} __packed EvKind_t;

typedef struct EventWaitNote {
  TAILQ_ENTRY(EventWaitNote) link;
  TaskHandle_t listener;
} EventWaitNote_t;

typedef TAILQ_HEAD(, EventWaitNote) EventWaitList_t;

void EventWaitListInit(EventWaitList_t *wl);
void EventNotifyFromISR(EventWaitList_t *wl);
int EventMonitor(EventWaitList_t *wl);
