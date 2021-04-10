#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <interrupt.h>
#include <custom.h>
#include <cia.h>
#include <device.h>
#include <input.h>
#include <ioreq.h>
#include <mouse.h>
#include <strings.h>
#include <libkern.h>
#include <sys/errno.h>

#define DEBUG 0
#include <debug.h>

typedef struct MouseDev {
  QueueHandle_t eventQ;
  SemaphoreHandle_t lock;
  IntServer_t intr;
  IoReq_t *io;
  int8_t xctr, yctr;
  uint8_t button;
} MouseDev_t;

static int MouseRead(Device_t *, IoReq_t *);

static DeviceOps_t MouseOps = {
  .read = MouseRead,
};

static int MouseRead(Device_t *dev, IoReq_t *io) {
  MouseDev_t *ms = dev->data;

  if (ms->io != io) {
    xSemaphoreTake(ms->lock, portMAX_DELAY);
    ms->io = io;
  }

  int error;
  if ((error = InputEventRead(ms->eventQ, io)))
    return error;

  /* Finish processing the request. */
  ms->io = NULL;
  xSemaphoreGive(ms->lock);

  return 0;
}

static uint8_t ReadButtonState(void) {
  uint8_t state = 0;

  if (!(ciaa.ciapra & CIAF_GAMEPORT0))
    state |= LMB;
  if (!(custom.potinp & DATLY))
    state |= RMB;

  return state;
}

static void MouseIntHandler(void *data) {
  MouseDev_t *ms = data;
  static InputEvent_t ev[4];
  size_t evcnt = 0;

  uint16_t joy0dat = custom.joy0dat;

  /* Register mouse position change first. */
  int8_t xctr = joy0dat;
  int8_t xrel = xctr - ms->xctr;

  if (xrel) {
    ev[evcnt++] = (InputEvent_t){.kind = IE_MOUSE_DELTA_X, .value = xrel};
    ms->xctr = xctr;
  }

  int8_t yctr = joy0dat >> 8;
  int8_t yrel = yctr - ms->yctr;

  if (yrel) {
    ev[evcnt++] = (InputEvent_t){.kind = IE_MOUSE_DELTA_Y, .value = yrel};
    ms->yctr = yctr;
  }

  /* After that a change in mouse button state. */
  uint8_t button = ReadButtonState();
  uint8_t change = ms->button ^ button;

  ms->button = button;

  if (change & LMB)
    ev[evcnt++] = (InputEvent_t){
      .kind = (button & LMB) ? IE_MOUSE_DOWN : IE_MOUSE_UP, .value = LMB};

  if (change & RMB)
    ev[evcnt++] = (InputEvent_t){
      .kind = (button & RMB) ? IE_MOUSE_DOWN : IE_MOUSE_UP, .value = RMB};

  if (evcnt > 0) {
    DPRINTF("mouse: inject %d events\n", evcnt);
    InputEventInjectFromISR(ms->eventQ, ev, evcnt);
    if (ms->io) {
      IoReqNotifyFromISR(ms->io);
      DPRINTF("mouse: send notification\n");
    }
  }
}

Device_t *MouseInit(void) {
  MouseDev_t *ms = kcalloc(1, sizeof(MouseDev_t));
  DASSERT(ms != NULL);

  klog("[Mouse] Initializing driver!\n");

  ms->lock = xSemaphoreCreateMutex();
  ms->eventQ = InputEventQueueCreate();

  /* Settings from MouseData structure. */
  ms->xctr = custom.joy0dat & 0xff;
  ms->yctr = custom.joy0dat >> 8;
  ms->button = ReadButtonState();

  ms->intr = INTSERVER(-5, MouseIntHandler, (void *)ms);
  AddIntServer(VertBlankChain, &ms->intr);

  return AddDeviceAux("mouse", &MouseOps, (void *)ms);
}
