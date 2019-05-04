#include <interrupt.h>
#include <custom.h>
#include <cia.h>
#include <mouse.h>
#include <event.h>
#include <strings.h>
#include <stdio.h>

typedef struct {
  int8_t xctr, yctr;
  int16_t x, y;
  int16_t button;

  int16_t left;
  int16_t right;
  int16_t top;
  int16_t bottom;
} MouseData_t;

static MouseData_t mouseData[1];

static inline bool GetMouseX(MouseData_t *mouse, MouseEvent_t *event) {
  int8_t xctr = custom.joy0dat & 0xff;
  int8_t xrel = xctr - mouse->xctr;
  int16_t x = mouse->x;

  if (!xrel) {
    event->x = x;
    return false;
  }

  x += xrel;

  if (x < mouse->left)
    x = mouse->left;
  if (x > mouse->right)
    x = mouse->right;

  event->x = x;
  event->xrel = xrel;

  mouse->x = x;
  mouse->xctr = xctr;

  return true;
}

static inline bool GetMouseY(MouseData_t *mouse, MouseEvent_t *event) {
  int8_t yctr = custom.joy0dat >> 8;
  int8_t yrel = yctr - mouse->yctr;
  int16_t y = mouse->y;

  if (!yrel) {
    event->y = y;
    return false;
  }

  y += yrel;

  if (y < mouse->top)
    y = mouse->top;
  if (y > mouse->bottom)
    y = mouse->bottom;

  event->y = y;
  event->yrel = yrel;

  mouse->y = y;
  mouse->yctr = yctr;

  return true;
}

static inline uint8_t ReadButtonState(void) {
  uint8_t state = 0;

  if (!(ciaa.ciapra & CIAF_GAMEPORT0))
    state |= LMB_PRESSED;
  if (!(custom.potinp & DATLY))
    state |= RMB_PRESSED;

  return state;
}

static inline bool GetMouseButton(MouseData_t *mouse, MouseEvent_t *event) {
  uint8_t button = ReadButtonState();
  uint8_t change = (mouse->button ^ button) & (LMB_PRESSED | RMB_PRESSED);

  if (!change)
    return false;

  mouse->button = button;
  event->button = 0;

  if (change & LMB_PRESSED)
    event->button |= (button & LMB_PRESSED) ? LMB_PRESSED : LMB_RELEASED;

  if (change & RMB_PRESSED)
    event->button |= (button & RMB_PRESSED) ? RMB_PRESSED : RMB_RELEASED;
  
  return true;
}

static void MouseIntHandler(void *data) {
  MouseData_t *mouse = (MouseData_t *)data;
  MouseEvent_t event;
  bool moveX, moveY;

  bzero(&event, sizeof(event));
  event.type = EV_MOUSE;

  /* Register mouse position change first. */
  moveX = GetMouseX(mouse, &event);
  moveY = GetMouseY(mouse, &event);

  if (moveX || moveY)
    PushEventFromISR((Event_t *)&event);

  /* After that a change in mouse button state. */
  if (GetMouseButton(mouse, &event)) {
    event.x = mouse->x;
    event.y = mouse->y;
    PushEventFromISR((Event_t *)&event);
  }
}

INTSERVER_DEFINE(MouseInterrupt, -5, MouseIntHandler, (void *)mouseData);

void MouseInit(int16_t minX, int16_t minY, int16_t maxX, int16_t maxY) {
  printf("[Init] Mouse driver!\n");

  MouseData_t *mouse = mouseData;

  *mouse = (MouseData_t){
    .left = minX,
    .right = maxX,
    .top = minY,
    .bottom = maxY,
    .x = minX,
    .y = minY,
    .xctr = custom.joy0dat & 0xff,
    .yctr = custom.joy0dat >> 8,
    .button = ReadButtonState()
  };

  AddIntServer(VertBlankChain, MouseInterrupt);
}

void MouseKill(void) {
  RemIntServer(MouseInterrupt);
}
