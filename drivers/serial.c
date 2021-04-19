#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <custom.h>
#include <interrupt.h>
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
  IntServer_t intr;
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
  .open = NullDevOpen,
  .close = NullDevClose,
  .read = SerialRead,
  .write = SerialWrite,
  .strategy = NullDevStrategy,
  .ioctl = NullDevIoctl,
  .event = SerialEvent,
  .seekable = false,
};

/* Handles full-duplex transmission. */
static void SerialTransmit(SerialDev_t *ser) {
  uint32_t ulSavedInterruptMask = portSET_INTERRUPT_MASK_FROM_ISR();

  for (;;) {
    /* serdatr contains both data and status bits! */
    uint16_t serdat = custom.serdatr;
    int16_t done = 0;

    if ((serdat & SERDATF_RBF) && !RingFull(ser->rxBuf)) {
      RingPutByte(ser->rxBuf, serdat);
      done++;
    }

    if ((serdat & (SERDATF_TBE | SERDATF_TSRE)) && !RingEmpty(ser->txBuf)) {
      uint8_t byte = RingGetByte(ser->txBuf);
      /* Send one byte into the wire. */
      custom.serdat = (uint16_t)(byte) | (uint16_t)0x100;
      done++;
    }

    if (!done)
      break;
  }

  portCLEAR_INTERRUPT_MASK_FROM_ISR(ulSavedInterruptMask);
}

static void SendIntHandler(void *ptr) {
  SerialDev_t *ser = ptr;
  SerialTransmit(ser);
  if (RingEmpty(ser->txBuf)) {
    if (ser->txTask) {
      NotifySendFromISR(ser->txTask, NB_IRQ);
      DLOG("serial: writer wakeup!\n");
    } else {
      EventNotifyFromISR(&ser->writeEvent);
      DLOG("serial: notify write listeners!\n");
    }
  }
}

static void RecvIntHandler(void *ptr) {
  SerialDev_t *ser = ptr;
  SerialTransmit(ser);
  if (!RingEmpty(ser->rxBuf)) {
    if (ser->rxTask) {
      NotifySendFromISR(ser->rxTask, NB_IRQ);
      DLOG("serial: reader wakeup!\n");
    } else {
      EventNotifyFromISR(&ser->readEvent);
      DLOG("serial: notify read listeners!\n");
    }
  }
}

static void RecoverIntHandler(void *ptr) {
  SerialDev_t *ser = ptr;
  SerialTransmit(ser);
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

  /* BUG(cahir): I've run into a bug that I had burned too many hours to
   * identify and I finally gave up. Seems like INTF_TBE bit in INTREQ is set
   * but it does not trigger an interrupt, so emptying transmit queue hangs in
   * the middle. I'm tempted to say it's the emulator bug, but reading through
   * fs-uae was not very fruitful and I run out of time to investigate it.
   *
   * Also playing with DisableINT / EnableINT with INTF_INTEN argument yields
   * some weird behaviour in SerialTransmit, though in this context they can
   * be used to replaced *INTERRUPT_MASK* macros. Which makes me believe that
   * there's something I fundamentally do not understand.
   *
   * Hence I introduced a recovery mechanism. At each VBlank I check if
   * transmission halted prematurely and I resume it. Hopefully that helps.
   */
  ser->intr = INTSERVER(-20, RecoverIntHandler, (void *)ser);
  AddIntServer(VertBlankChain, &ser->intr);

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

  RemIntServer(&ser->intr);

  vSemaphoreDelete(ser->rxLock);
  vSemaphoreDelete(ser->txLock);

  return 0;
}

static int SerialWrite(DevFile_t *dev, IoReq_t *req) {
  SerialDev_t *ser = dev->data;
  size_t n = req->left;
  int error = 0;

  Assert(n > 0);

  xSemaphoreTake(ser->txLock, portMAX_DELAY);
  taskENTER_CRITICAL();

  ser->txTask = xTaskGetCurrentTaskHandle();

  /* Write all data to transmit buffer. This may involve waiting for the
   * interupt handler to free enough space in the ring buffer. */
  do {
    RingWrite(ser->txBuf, req);
    SerialTransmit(ser);
    if (!req->left)
      break;
    if (req->flags & F_NONBLOCK) {
      /* Nonblocking mode: if the request was partially filled then return with
       * short count, otherwise signify that we would have blocked and leave. */
      if (req->left < n)
        break;
      DLOG("serial: tx buffer full!\n");
      error = EAGAIN;
      break;
    }
  } while (NotifyWait(NB_IRQ, portMAX_DELAY));

  ser->txTask = NULL;

  DLOG("serial: write request done; wrote %d bytes!\n", n - req->left);
  taskEXIT_CRITICAL();
  xSemaphoreGive(ser->txLock);

  return error;
}

static int SerialRead(DevFile_t *dev, IoReq_t *req) {
  SerialDev_t *ser = dev->data;
  int error = 0;
  __unused size_t n = req->left;

  xSemaphoreTake(ser->rxLock, portMAX_DELAY);
  taskENTER_CRITICAL();

  ser->rxTask = xTaskGetCurrentTaskHandle();

  /* Wait for the interrupt handler to put data into the ring buffer. */
  while (RingEmpty(ser->rxBuf)) {
    /* Nonblocking mode: if there's no data in the ring buffer signify that
     * we would have blocked and leave. */
    if (req->flags & F_NONBLOCK) {
      error = EAGAIN;
      break;
    }
    (void)NotifyWait(NB_IRQ, portMAX_DELAY);
  }

  ser->rxTask = NULL;

  if (!error)
    RingRead(ser->rxBuf, req);

  DLOG("serial: rx request done; read %d bytes!\n", n - req->left);
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
