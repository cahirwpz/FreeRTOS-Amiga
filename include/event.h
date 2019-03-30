#ifndef _EVENT_H_
#define _EVENT_H_

#include <mouse.h>

#define EV_UNKNOWN 0
#define EV_MOUSE 1

typedef union Event {
  uint8_t type;
  MouseEvent_t mouse;
} Event_t;

void EventQueueInit(void);
void EventQueueKill(void);

void PushEvent(Event_t *ev);
void PushEventFromISR(Event_t *ev);
bool PopEvent(Event_t *ev);

#endif /* !_EVENT_H_ */
