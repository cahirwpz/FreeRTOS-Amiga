#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <custom.h>
#include <interrupt.h>
#include <libkern.h>
#include <serial.h>
#include <ring.h>
#include <event.h>
#include <notify.h>
#include <ioreq.h>
#include <device.h>
#include <sys/errno.h>

#define DEBUG 0
#include <debug.h>

#define CLOCK 3546895
#define BUFLEN 64

typedef struct SerialDev {
  SemaphoreHandle_t rxLock;
  SemaphoreHandle_t txLock;
  Ring_t *rxBuf;
  Ring_t *txBuf;
  TaskHandle_t rxTask;
  TaskHandle_t txTask;
  EventWaitList_t readEvent;
  EventWaitList_t writeEvent;
} SerialDev_t;

static SerialDev_t SerialDev[1];

static int SerialRead(Device_t *, IoReq_t *);
static int SerialWrite(Device_t *, IoReq_t *);
static int SerialEvent(Device_t *, EvKind_t);

static DeviceOps_t SerialOps = {
  .read = SerialRead,
  .write = SerialWrite,
  .event = SerialEvent,
};

#define SendByte(byte)                                                         \
  { custom.serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static void SendIntHandler(void *ptr) {
  SerialDev_t *ser = ptr;
  /* Send one byte into the wire. */
  int byte = RingGetByte(ser->txBuf);
  if (byte >= 0) {
    SendByte(byte);
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
  RingPutByte(ser->rxBuf, code);
  if (ser->rxTask) {
    NotifySendFromISR(ser->rxTask, NB_IRQ);
    DPRINTF("serial: reader wakeup!\n");
  } else {
    EventNotifyFromISR(&ser->readEvent);
    DPRINTF("serial: notify read listeners!\n");
  }
}

Device_t *SerialInit(unsigned baud) {
  SerialDev_t *ser = SerialDev;

  klog("[Serial] Initializing driver!\n");

  custom.serper = CLOCK / baud - 1;

  ser->rxLock = xSemaphoreCreateMutex();
  ser->txLock = xSemaphoreCreateMutex();
  ser->rxBuf = RingAlloc(BUFLEN);
  ser->txBuf = RingAlloc(BUFLEN);

  EventWaitListInit(&ser->readEvent);
  EventWaitListInit(&ser->writeEvent);

  SetIntVec(TBE, SendIntHandler, ser);
  SetIntVec(RBF, RecvIntHandler, ser);

  ClearIRQ(INTF_TBE | INTF_RBF);
  EnableINT(INTF_TBE | INTF_RBF);

  return AddDeviceAux("serial", &SerialOps, SerialDev);
}

void SerialKill(void) {
  DisableINT(INTF_TBE | INTF_RBF);
  ResetIntVec(TBE);
  ResetIntVec(RBF);

  vSemaphoreDelete(SerialDev->rxLock);
  vSemaphoreDelete(SerialDev->txLock);

  klog("[Serial] Driver deactivated!\n");
}

static int SerialWrite(Device_t *dev, IoReq_t *req) {
  SerialDev_t *ser = dev->data;
  size_t n = req->left;
  int error = 0;

  xSemaphoreTake(ser->txLock, portMAX_DELAY);
  taskENTER_CRITICAL();

  ser->txTask = xTaskGetCurrentTaskHandle();

  /* Write all data to transmit buffer. This may involve waiting for the
   * interupt handler to free enough space in the ring buffer. */
  while (NotifyWait(NB_IRQ, portMAX_DELAY)) {
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
  }

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

static int SerialRead(Device_t *dev, IoReq_t *req) {
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

static int SerialEvent(Device_t *dev, EvKind_t ev) {
  SerialDev_t *ser = dev->data;

  if (ev == EV_READ)
    return EventMonitor(&ser->readEvent);
  if (ev == EV_WRITE)
    return EventMonitor(&ser->writeEvent);
  return EINVAL;
}
