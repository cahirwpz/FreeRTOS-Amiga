#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <custom.h>
#include <interrupt.h>
#include <libkern.h>
#include <serial.h>
#include <ring.h>
#include <ioreq.h>
#include <device.h>
#include <sys/errno.h>

#define CLOCK 3546895
#define BUFLEN 64

typedef struct SerialDev {
  SemaphoreHandle_t rxLock;
  SemaphoreHandle_t txLock;
  Ring_t *rxBuf;
  Ring_t *txBuf;
  IoReq_t *rxReq;
  IoReq_t *txReq;
} SerialDev_t;

static SerialDev_t SerialDev[1];

static int SerialRead(Device_t *, IoReq_t *);
static int SerialWrite(Device_t *, IoReq_t *);

static DeviceOps_t SerialOps = {
  .read = SerialRead,
  .write = SerialWrite,
};

#define SendByte(byte)                                                         \
  { custom.serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static void SendIntHandler(void *ptr) {
  SerialDev_t *ser = ptr;
  /* Send one byte into the wire. */
  int byte = RingGetByte(ser->txBuf);
  if (byte >= 0)
    SendByte(byte);
  if (RingEmpty(ser->txBuf) && ser->txReq)
    IoReqNotifyFromISR(ser->txReq);
}

static void RecvIntHandler(void *ptr) {
  /* serdatr contains both data and status bits! */
  uint16_t code = custom.serdatr;
  if (!(code & SERDATF_RBF))
    return;
  SerialDev_t *ser = ptr;
  RingPutByte(ser->rxBuf, code);
  if (ser->rxReq)
    IoReqNotifyFromISR(ser->rxReq);
}

Device_t *SerialInit(unsigned baud) {
  klog("[Serial] Initializing driver!\n");

  custom.serper = CLOCK / baud - 1;

  SerialDev->rxLock = xSemaphoreCreateMutex();
  SerialDev->txLock = xSemaphoreCreateMutex();
  SerialDev->rxBuf = RingAlloc(BUFLEN);
  SerialDev->txBuf = RingAlloc(BUFLEN);

  SetIntVec(TBE, SendIntHandler, SerialDev);
  SetIntVec(RBF, RecvIntHandler, SerialDev);

  ClearIRQ(INTF_TBE | INTF_RBF);
  EnableINT(INTF_TBE | INTF_RBF);

  Device_t *dev;
  AddDevice("serial", &SerialOps, &dev);
  dev->data = SerialDev;
  return dev;
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

  /* Should continue processing asynchronous request? */
  if (ser->txReq == req) {
    taskENTER_CRITICAL();
  } else {
    xSemaphoreTake(ser->txLock, portMAX_DELAY);
    taskENTER_CRITICAL();
    ser->txReq = req;
  }

  /* Trigger interrupt if serdat register and TxBuf are empty. */
  if (RingEmpty(ser->txBuf) && custom.serdatr & SERDATF_TBE)
    CauseIRQ(INTF_TBE);

  /* Write all data to transmit buffer. This may involve waiting for the
   * interupt handler to free enough space in the ring buffer. */
  for (;;) {
    RingWrite(ser->txBuf, req);
    if (!req->left)
      break;
    if (req->async) {
      /* Asynchronous mode: if the request was partially filled then return with
       * short count, otherwise signify that we would have blocked and leave
       * without finishing the request. */
      if (req->left < n)
        break;
      taskEXIT_CRITICAL();
      return EAGAIN;
    }
    IoReqNotifyWait(req, NULL);
  }

  /* Finish processing the request. */
  ser->txReq = NULL;
  taskEXIT_CRITICAL();
  xSemaphoreGive(ser->txLock);

  return 0;
}

static int SerialRead(Device_t *dev, IoReq_t *req) {
  SerialDev_t *ser = dev->data;

  /* Should continue processing asynchronous request? */
  if (ser->rxReq == req) {
    taskENTER_CRITICAL();
  } else {
    xSemaphoreTake(ser->rxLock, portMAX_DELAY);
    taskENTER_CRITICAL();
    ser->rxReq = req;
  }

  /* Wait for the interrupt handler to put data into the ring buffer. */
  while (RingEmpty(ser->rxBuf)) {
    if (req->async) {
      /* Asynchronous mode: if there's no data in the ring buffer signify that
       * we would have blocked and leave without finishing the request. */
      taskEXIT_CRITICAL();
      return EAGAIN;
    }
    IoReqNotifyWait(req, NULL);
  }

  /* Finish processing the request. */
  RingRead(ser->rxBuf, req);
  ser->rxReq = NULL;
  taskEXIT_CRITICAL();
  xSemaphoreGive(ser->rxLock);

  return 0;
}
