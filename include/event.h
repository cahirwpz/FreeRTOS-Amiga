#ifndef _EVENT_H_
#define _EVENT_H_

#include <mouse.h>
#include <keyboard.h>

#define EV_UNKNOWN 0
#define EV_MOUSE 1
#define EV_KEY 2

typedef union Event {
  uint8_t type;
  MouseEvent_t mouse;
  KeyEvent_t key;
} Event_t;

void EventQueueInit(void);
void EventQueueKill(void);

void PushEvent(Event_t *ev);
void PushEventFromISR(Event_t *ev);
bool PopEvent(Event_t *ev);

#endif /* !_EVENT_H_ */
