#include <interrupt.h>
#include <custom.h>
#include <cia.h>
#include <devfile.h>
#include <event.h>
#include <input.h>
#include <ioreq.h>
#include <driver.h>
#include <strings.h>
#include <sys/errno.h>

#define DEBUG 0
#include <debug.h>

typedef struct MouseDev {
  QueueHandle_t eventQ;
  IntServer_t intr;
  DevFile_t *file;
  EventWaitList_t readEvent;
  int8_t xctr, yctr;
  uint8_t button;
} MouseDev_t;

static int MouseOpen(DevFile_t *, FileFlags_t);
static int MouseClose(DevFile_t *, FileFlags_t);
static int MouseRead(DevFile_t *, IoReq_t *);
static int MouseEvent(DevFile_t *, EvAction_t, EvFilter_t);

static DevFileOps_t MouseOps = {
  .type = DT_OTHER,
  .open = MouseOpen,
  .close = MouseClose,
  .read = MouseRead,
  .event = MouseEvent,
};

static uint8_t ReadButtonState(void) {
  uint8_t state = 0;

  if (!(ciaa.ciapra & CIAF_GAMEPORT0))
    state |= LMB;
  if (!(custom.potinp & DATLY))
    state |= RMB;

  return state;
}

static int MouseOpen(DevFile_t *dev, FileFlags_t flags) {
  if (flags & F_WRITE)
    return EACCES;

  if (!dev->usecnt) {
    MouseDev_t *ms = dev->data;

    ms->eventQ = InputEventQueueCreate();
    ms->xctr = custom.joy0dat & 0xff;
    ms->yctr = custom.joy0dat >> 8;
    ms->button = ReadButtonState();

    AddIntServer(VertBlankChain, &ms->intr);
  }

  return 0;
}

static int MouseClose(DevFile_t *dev, FileFlags_t flags __unused) {
  if (!dev->usecnt) {
    MouseDev_t *ms = dev->data;

    RemIntServer(&ms->intr);
    InputEventQueueDelete(ms->eventQ);
  }

  return 0;
}

static int MouseRead(DevFile_t *dev, IoReq_t *io) {
  MouseDev_t *ms = dev->data;
  return InputEventRead(ms->eventQ, io);
}

static int MouseEvent(DevFile_t *dev, EvAction_t act, EvFilter_t filt) {
  MouseDev_t *ms = dev->data;

  if (filt == EVFILT_READ)
    return EventMonitor(&ms->readEvent, act);
  return EINVAL;
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
    DLOG("mouse: inject %d events\n", evcnt);
    InputEventInjectFromISR(ms->eventQ, ev, evcnt);
    EventNotifyFromISR(&ms->readEvent);
    DLOG("mouse: notify read listeners!\n");
  }
}

static int MouseAttach(Driver_t *drv) {
  MouseDev_t *ms = drv->state;

  TAILQ_INIT(&ms->readEvent);
  ms->intr = INTSERVER(-5, MouseIntHandler, (void *)ms);

  int error;
  if ((error = AddDevFile("mouse", &MouseOps, &ms->file)))
    return error;

  ms->file->data = ms;
  return error;
}

Driver_t Mouse = {
  .name = "mouse",
  .attach = MouseAttach,
  .size = sizeof(MouseDev_t),
};
