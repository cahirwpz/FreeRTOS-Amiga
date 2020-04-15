#include <interrupt.h>
#include <custom.h>
#include <cia.h>
#include <mouse.h>
#include <strings.h>
#include <stdio.h>

typedef struct {
  MouseEvent_t event;

  int8_t xctr, yctr;
  uint8_t button;

  int16_t left;
  int16_t right;
  int16_t top;
  int16_t bottom;
} MouseData_t;

static MouseData_t MouseData;
static MouseEventNotify_t MouseEventNotify;

static bool GetMouseMove(MouseData_t *mouse) {
  uint16_t joy0dat = custom.joy0dat;

  int8_t xctr = joy0dat;
  int8_t xrel = xctr - mouse->xctr;

  if (xrel) {
    int16_t x = mouse->event.x + xrel;

    if (x < mouse->left)
      x = mouse->left;
    if (x > mouse->right)
      x = mouse->right;

    mouse->event.x = x;
    mouse->event.xrel = xrel;
    mouse->xctr = xctr;
  }

  int8_t yctr = joy0dat >> 8;
  int8_t yrel = yctr - mouse->yctr;

  if (yrel) {
    int16_t y = mouse->event.y + yrel;

    if (y < mouse->top)
      y = mouse->top;
    if (y > mouse->bottom)
      y = mouse->bottom;

    mouse->event.y = y;
    mouse->event.yrel = yrel;
    mouse->yctr = yctr;
  }

  return xrel || yrel;
}

static uint8_t ReadButtonState(void) {
  uint8_t state = 0;

  if (!(ciaa.ciapra & CIAF_GAMEPORT0))
    state |= LMB_PRESSED;
  if (!(custom.potinp & DATLY))
    state |= RMB_PRESSED;

  return state;
}

static bool GetMouseButton(MouseData_t *mouse) {
  uint8_t button = ReadButtonState();
  uint8_t change = (mouse->button ^ button) & (LMB_PRESSED | RMB_PRESSED);

  if (!change)
    return false;

  mouse->button = button;

  if (change & LMB_PRESSED)
    mouse->event.button |= (button & LMB_PRESSED) ? LMB_PRESSED : LMB_RELEASED;

  if (change & RMB_PRESSED)
    mouse->event.button |= (button & RMB_PRESSED) ? RMB_PRESSED : RMB_RELEASED;

  return true;
}

static void MouseIntHandler(void *data) {
  MouseData_t *mouse = (MouseData_t *)data;

  mouse->event.button = 0;

  /* Register mouse position change first. */
  if (GetMouseMove(mouse))
    MouseEventNotify(&mouse->event);

  /* After that a change in mouse button state. */
  if (GetMouseButton(mouse))
    MouseEventNotify(&mouse->event);
}

INTSERVER_DEFINE(MouseInterrupt, -5, MouseIntHandler, (void *)&MouseData);

void MouseInit(MouseEventNotify_t notify,
               int16_t minX, int16_t minY, int16_t maxX, int16_t maxY) {
  printf("[Init] Mouse driver!\n");

  /* Register notification procedure called from ISR */
  MouseEventNotify = notify;

  /* Settings from MouseData structure. */
  MouseData.left = minX;
  MouseData.right = maxX;
  MouseData.top = minY;
  MouseData.bottom = maxY;
  MouseData.xctr = custom.joy0dat & 0xff;
  MouseData.yctr = custom.joy0dat >> 8;
  MouseData.button = ReadButtonState();
  MouseData.event.x = minX;
  MouseData.event.y = minY;
  MouseData.event.type = EV_MOUSE;

  AddIntServer(VertBlankChain, MouseInterrupt);
}

void MouseKill(void) {
  RemIntServer(MouseInterrupt);
  MouseEventNotify = NULL;
}
