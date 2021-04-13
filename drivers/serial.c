#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <custom.h>
#include <interrupt.h>
#include <libkern.h>
#include <driver.h>
#include <ring.h>
#include <event.h>
#include <notify.h>
#include <ioreq.h>
#include <devfile.h>
#include <sys/errno.h>

#define DEBUG 0
#include <debug.h>

#define CLOCK 3546895
#define BUFLEN 64

typedef struct SerialDev {
  DevFile_t *file;
  SemaphoreHandle_t rxLock;
  SemaphoreHandle_t txLock;
  Ring_t *rxBuf;
  Ring_t *txBuf;
  TaskHandle_t rxTask;
  TaskHandle_t txTask;
  EventWaitList_t readEvent;
  EventWaitList_t writeEvent;
} SerialDev_t;

static int SerialRead(DevFile_t *, IoReq_t *);
static int SerialWrite(DevFile_t *, IoReq_t *);
static int SerialEvent(DevFile_t *, EvKind_t);

static DevFileOps_t SerialOps = {
  .read = SerialRead,
  .write = SerialWrite,
  .event = SerialEvent,
};

#define SendByte(byte)                                                         \
  { custom.serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static void SendIntHandler(void *ptr) {
  SerialDev_t *ser = ptr;
  /* Send one byte into the wire. */
  if (!RingEmpty(ser->txBuf)) {
    SendByte(RingGetByte(ser->txBuf));
  } else if (ser->txTask) {
    NotifySendFromISR(ser->txTask, NB_IRQ);
    DPRINTF("serial: writer wakeup!\n");
  } else {
    EventNotifyFromISR(&ser->writeEvent);
    DPRINTF("serial: notify write listeners!\n");
  }
}

static void RecvIntHandler(void *ptr) {
  /* serdatr contains both data and status bits! */
  uint16_t code = custom.serdatr;
  if (!(code & SERDATF_RBF))
    return;
  SerialDev_t *ser = ptr;
  if (!RingFull(ser->rxBuf))
    RingPutByte(ser->rxBuf, code);
  if (ser->rxTask) {
    NotifySendFromISR(ser->rxTask, NB_IRQ);
    DPRINTF("serial: reader wakeup!\n");
  } else {
    EventNotifyFromISR(&ser->readEvent);
    DPRINTF("serial: notify read listeners!\n");
  }
}

static int SerialAttach(Driver_t *drv) {
  SerialDev_t *ser = drv->state;
  unsigned baud = 9600;
  int error;

  custom.serper = CLOCK / baud - 1;

  ser->rxLock = xSemaphoreCreateMutex();
  ser->txLock = xSemaphoreCreateMutex();
  ser->rxBuf = RingAlloc(BUFLEN);
  ser->txBuf = RingAlloc(BUFLEN);

  TAILQ_INIT(&ser->readEvent);
  TAILQ_INIT(&ser->writeEvent);

  SetIntVec(TBE, SendIntHandler, ser);
  SetIntVec(RBF, RecvIntHandler, ser);

  ClearIRQ(INTF_TBE | INTF_RBF);
  EnableINT(INTF_TBE | INTF_RBF);

  error = AddDevFile("serial", &SerialOps, &ser->file);
  ser->file->data = (void *)ser;
  return error;
}

int SerialDetach(Driver_t *drv) {
  SerialDev_t *ser = drv->state;

  DisableINT(INTF_TBE | INTF_RBF);
  ResetIntVec(TBE);
  ResetIntVec(RBF);

  vSemaphoreDelete(ser->rxLock);
  vSemaphoreDelete(ser->txLock);

  return 0;
}

static int SerialWrite(DevFile_t *dev, IoReq_t *req) {
  SerialDev_t *ser = dev->data;
  size_t n = req->left;
  int error = 0;

  xSemaphoreTake(ser->txLock, portMAX_DELAY);
  taskENTER_CRITICAL();

  ser->txTask = xTaskGetCurrentTaskHandle();

  /* Write all data to transmit buffer. This may involve waiting for the
   * interupt handler to free enough space in the ring buffer. */
  do {
    RingWrite(ser->txBuf, req);
    if (!req->left)
      break;
    if (req->nonblock) {
      /* Nonblocking mode: if the request was partially filled then return with
       * short count, otherwise signify that we would have blocked and leave. */
      if (req->left < n)
        break;
      DPRINTF("serial: tx buffer full!\n");
      error = EAGAIN;
      break;
    }
  } while (NotifyWait(NB_IRQ, portMAX_DELAY));

  ser->txTask = NULL;

  /* Trigger interrupt if transmit buffer is empty. */
  if (custom.serdatr & SERDATF_TBE) {
    CauseIRQ(INTF_TBE);
    DPRINTF("serial: initiate tx!\n");
  }

  DPRINTF("serial: tx request finished!\n");
  taskEXIT_CRITICAL();
  xSemaphoreGive(ser->txLock);

  return error;
}

static int SerialRead(DevFile_t *dev, IoReq_t *req) {
  SerialDev_t *ser = dev->data;
  int error = 0;

  xSemaphoreTake(ser->rxLock, portMAX_DELAY);
  taskENTER_CRITICAL();

  ser->rxTask = xTaskGetCurrentTaskHandle();

  /* Wait for the interrupt handler to put data into the ring buffer. */
  while (RingEmpty(ser->rxBuf)) {
    /* Nonblocking mode: if there's no data in the ring buffer signify that
     * we would have blocked and leave. */
    if (req->nonblock) {
      error = EAGAIN;
      break;
    }
    (void)NotifyWait(NB_IRQ, portMAX_DELAY);
  }

  ser->rxTask = NULL;

  if (!error)
    RingRead(ser->rxBuf, req);

  DPRINTF("serial: rx request finished!\n");
  taskEXIT_CRITICAL();
  xSemaphoreGive(ser->rxLock);

  return error;
}

static int SerialEvent(DevFile_t *dev, EvKind_t ev) {
  SerialDev_t *ser = dev->data;

  if (ev == EV_READ)
    return EventMonitor(&ser->readEvent);
  if (ev == EV_WRITE)
    return EventMonitor(&ser->writeEvent);
  return EINVAL;
}

Driver_t Serial = {
  .name = "serial",
  .attach = SerialAttach,
  .detach = SerialDetach,
  .size = sizeof(SerialDev_t),
};
