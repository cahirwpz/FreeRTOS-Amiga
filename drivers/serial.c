#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <custom.h>
#include <interrupt.h>
#include <libkern.h>
#include <serial.h>
#include <ring.h>
#include <ioreq.h>
#include <device.h>

#define CLOCK 3546895
#define BUFLEN 64

static SemaphoreHandle_t TxLock;
static SemaphoreHandle_t RxLock;
static Ring_t *TxBuf = &RING(BUFLEN);
static Ring_t *RxBuf = &RING(BUFLEN);
static TaskHandle_t TxWaiter;
static TaskHandle_t RxWaiter;

static int SerialRead(Device_t *, IoReq_t *);
static int SerialWrite(Device_t *, IoReq_t *);

static DeviceOps_t SerialOps = {
  .read = SerialRead,
  .write = SerialWrite,
};

#define SendByte(byte)                                                         \
  { custom.serdat = (uint16_t)(byte) | (uint16_t)0x100; }

static void SendIntHandler(__unused void *ptr) {
  /* Send one byte into the wire. */
  int byte = RingGetByte(TxBuf);
  if (byte >= 0)
    SendByte(byte);
  if (RingEmpty(TxBuf) && TxWaiter)
    xTaskNotifyFromISR(TxWaiter, 0, eNoAction, &xNeedRescheduleTask);
}

static void RecvIntHandler(__unused void *ptr) {
  /* serdatr contains both data and status bits! */
  uint16_t code = custom.serdatr;
  if (code & SERDATF_RBF) {
    RingPutByte(RxBuf, code);
    if (RxWaiter)
      xTaskNotifyFromISR(RxWaiter, 0, eNoAction, &xNeedRescheduleTask);
  }
}

Device_t *SerialInit(unsigned baud) {
  kprintf("[Serial] Initializing driver!\n");

  custom.serper = CLOCK / baud - 1;

  RxLock = xSemaphoreCreateMutex();
  TxLock = xSemaphoreCreateMutex();

  SetIntVec(TBE, SendIntHandler, NULL);
  SetIntVec(RBF, RecvIntHandler, NULL);

  ClearIRQ(INTF_TBE | INTF_RBF);
  EnableINT(INTF_TBE | INTF_RBF);

  Device_t *dev;
  AddDevice("serial", &SerialOps, &dev);
  return dev;
}

void SerialKill(void) {
  DisableINT(INTF_TBE | INTF_RBF);
  ResetIntVec(TBE);
  ResetIntVec(RBF);

  vSemaphoreDelete(RxLock);
  vSemaphoreDelete(TxLock);

  kprintf("[Serial] Driver deactivated!\n");
}

static int SerialWrite(Device_t *dev __unused, IoReq_t *req) {
  xSemaphoreTake(TxLock, portMAX_DELAY);
  taskENTER_CRITICAL();
  {
    /* Trigger interrupt if serdat register and TxBuf are empty. */
    if (RingEmpty(TxBuf) && custom.serdatr & SERDATF_TBE)
      CauseIRQ(INTF_TBE);

    /* Write all bytes to transmit buffer. */
    for (;;) {
      RingWrite(TxBuf, req);
      if (!req->left)
        break;
      TxWaiter = xTaskGetCurrentTaskHandle();
      xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    }
  }
  taskEXIT_CRITICAL();
  xSemaphoreGive(TxLock);

  return 0;
}

static int SerialRead(Device_t *dev __unused, IoReq_t *req) {
  xSemaphoreTake(RxLock, portMAX_DELAY);
  taskENTER_CRITICAL();
  {
    while (RingEmpty(RxBuf)) {
      RxWaiter = xTaskGetCurrentTaskHandle();
      xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
    }
    RingRead(RxBuf, req);
  }
  taskEXIT_CRITICAL();
  xSemaphoreGive(RxLock);

  return 0;
}
