#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <tty.h>
#include <device.h>
#include <ioreq.h>
#include <libkern.h>
#include <sys/errno.h>

/* TODO Implement rudimentary canonical mode, i.e. character echoing,
 * rewriting '\n' to '\r\n' on output, line buffering and character deletion. */

typedef struct TtyState {
  TaskHandle_t task;
  Device_t *cons;
  /* Used for communication with file-like objects. */
  IoReq_t *writeReq;
  SemaphoreHandle_t writeLock;
  IoReq_t *readReq;
  SemaphoreHandle_t readLock;
  /* Used for communication with hardware terminal. */
  IoReq_t rxReq;
  IoReq_t txReq;
} TtyState_t;

static void TtyTask(void *);
static int TtyRead(Device_t *, IoReq_t *);
static int TtyWrite(Device_t *, IoReq_t *);

static DeviceOps_t TtyOps = {
  .read = TtyRead,
  .write = TtyWrite,
};

#define INPUTQ BIT(0)
#define OUTPUTQ BIT(1)

#define TTY_TASK_PRIO 2

int AddTtyDevice(const char *name, Device_t *cons) {
  Device_t *dev;
  TtyState_t *tty;
  int error;

  if (!(tty = kmalloc(sizeof(TtyState_t))))
    return ENOMEM;
  tty->cons = cons;
  tty->readLock = xSemaphoreCreateMutex();
  tty->writeLock = xSemaphoreCreateMutex();

  if ((error = AddDevice(name, &TtyOps, &dev)))
    return error;
  dev->data = (void *)tty;

  xTaskCreate((TaskFunction_t)TtyTask, name, configMINIMAL_STACK_SIZE, tty,
              TTY_TASK_PRIO, &tty->task);

  tty->rxReq.origin = tty->txReq.origin = tty->task;
  tty->rxReq.async = tty->txReq.async = 1;
  tty->rxReq.notifyBits = INPUTQ;
  tty->txReq.notifyBits = OUTPUTQ;

  return 0;
}

static inline int ConsGetChar(Device_t *cons, IoReq_t *req, char *data) {
  req->rbuf = data;
  req->left = 1;
  return cons->ops->read(cons, req);
}

static void HandleReadReq(TtyState_t *tty) {
  Device_t *cons = tty->cons;
  IoReq_t *req = tty->readReq;

  while (req->left) {
    if ((req->error = ConsGetChar(cons, &tty->rxReq, req->rbuf)))
      break;
    char ch = *req->rbuf++;
    req->left--;
    if (ch == '\n')
      break;
  }

  if (req->error == EAGAIN)
    return;

  tty->readReq = NULL;
  IoReqNotify(req);
}

static inline int ConsPutChar(Device_t *cons, IoReq_t *req, const char *data) {
  req->wbuf = data;
  req->left = 1;
  return cons->ops->write(cons, req);
}

static void HandleWriteReq(TtyState_t *tty) {
  Device_t *cons = tty->cons;
  IoReq_t *req = tty->writeReq;

  while (req->left) {
    if (*req->wbuf == '\n') {
      /* Turn LF into CR + LF */
      if ((req->error = ConsPutChar(cons, &tty->txReq, "\r")))
        break;
    }
    if ((req->error = ConsPutChar(cons, &tty->txReq, req->wbuf)))
      break;
    req->wbuf++;
    req->left--;
  }

  if (req->error == EAGAIN)
    return;

  tty->writeReq = NULL;
  IoReqNotify(req);
}

static void TtyTask(void *data) {
  TtyState_t *tty __unused = data;

  for (;;) {
    uint32_t value;
    xTaskNotifyWait(0, INPUTQ | OUTPUTQ, &value, portMAX_DELAY);
    if (value & INPUTQ)
      HandleReadReq(tty);
    if (value & OUTPUTQ)
      HandleWriteReq(tty);
  }
}

static int TtyWrite(Device_t *dev, IoReq_t *req) {
  TtyState_t *tty = dev->data;
  size_t n = req->left;

  xSemaphoreTake(tty->writeLock, portMAX_DELAY);
  {
    tty->writeReq = req;
    xTaskNotify(tty->task, OUTPUTQ, eSetBits);
    IoReqNotifyWait(req, NULL);
  }
  xSemaphoreGive(tty->writeLock);

  return req->left < n ? 0 : req->error;
}

static int TtyRead(Device_t *dev, IoReq_t *req) {
  TtyState_t *tty = dev->data;
  size_t n = req->left;

  xSemaphoreTake(tty->readLock, portMAX_DELAY);
  {
    tty->readReq = req;
    xTaskNotify(tty->task, INPUTQ, eSetBits);
    IoReqNotifyWait(req, NULL);
  }
  xSemaphoreGive(tty->readLock);

  return req->left < n ? 0 : req->error;
}
