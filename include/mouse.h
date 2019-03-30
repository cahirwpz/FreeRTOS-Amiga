#ifndef _MOUSE_H_
#define _MOUSE_H_

#include <cdefs.h>

#define LMB_PRESSED  BIT(0)
#define RMB_PRESSED  BIT(1)
#define LMB_RELEASED BIT(2)
#define RMB_RELEASED BIT(3)

typedef struct MouseEvent {
  uint8_t type;
  uint8_t button;
  int16_t x, y;
  int8_t xrel, yrel;
} MouseEvent_t;

void MouseInit(int16_t minX, int16_t minY, int16_t maxX, int16_t maxY);
void MouseKill(void);

#endif /* !_MOUSE_H_ */
