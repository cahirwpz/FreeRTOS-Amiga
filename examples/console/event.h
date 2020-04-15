#ifndef _EVENT_H_
#define _EVENT_H_

#include <mouse.h>
#include <keyboard.h>

typedef union Event {
  uint8_t type;
  MouseEvent_t mouse;
  KeyEvent_t key;
} Event_t;

void EventQueueInit(void);
void EventQueueKill(void);

void PushKeyEventFromISR(const KeyEvent_t *ev);
void PushMouseEventFromISR(const MouseEvent_t *ev);
bool PopEvent(Event_t *ev);

#endif /* !_EVENT_H_ */
