#ifndef _MOUSE_H_
#define _MOUSE_H_

#include <cdefs.h>

#define LMB_PRESSED  BIT(0)
#define RMB_PRESSED  BIT(1)
#define LMB_RELEASED BIT(2)
#define RMB_RELEASED BIT(3)

#define EV_MOUSE 1

typedef struct MouseEvent {
  uint8_t type;      /* always set to EV_MOUSE */
  uint8_t button;
  int16_t x, y;
  int8_t xrel, yrel;
} MouseEvent_t;

typedef void (*MouseEventNotify_t)(const MouseEvent_t *);

/* Notify procedure will always be called from ISR! */
void MouseInit(MouseEventNotify_t notify,
               int16_t minX, int16_t minY, int16_t maxX, int16_t maxY);
void MouseKill(void);

#endif /* !_MOUSE_H_ */
