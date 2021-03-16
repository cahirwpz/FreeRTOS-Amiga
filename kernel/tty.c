#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <tty.h>
#include <device.h>
#include <ioreq.h>
#include <libkern.h>
#include <sys/errno.h>

/* TODO Implement rudimentary canonical mode, i.e. character echoing,
 * rewriting '\n' to '\r\n' on output, line buffering and character deletion. */

#define LINEBUF_SIZE 256

typedef struct TtyState {
  Device_t *cons;
  SemaphoreHandle_t writeLock;
  SemaphoreHandle_t readLock;
} TtyState_t;

static int TtyRead(Device_t *, IoReq_t *);
static int TtyWrite(Device_t *, IoReq_t *);

static DeviceOps_t TtyOps = {
  .read = TtyRead,
  .write = TtyWrite,
};

int AddTtyDevice(const char *name, Device_t *cons) {
  Device_t *dev;
  TtyState_t *ts;
  int error;

  if (!(ts = kmalloc(sizeof(TtyState_t))))
    return ENOMEM;
  ts->cons = cons;
  ts->readLock = xSemaphoreCreateMutex();
  ts->writeLock = xSemaphoreCreateMutex();

  if ((error = AddDevice(name, &TtyOps, &dev)))
    return error;
  dev->data = (void *)ts;
  return 0;
}

static inline int ConsPutChar(Device_t *cons, char data) {
  return cons->ops->write(cons, &IOREQ_WRITE(0, &data, 1));
}

static int TtyWrite(Device_t *dev, IoReq_t *req) {
  TtyState_t *tty = dev->data;
  Device_t *cons = tty->cons;
  size_t n = req->left;
  int error;

  xSemaphoreTake(tty->writeLock, portMAX_DELAY);
  while (req->left) {
    char ch = *req->wbuf;
    if (ch == '\n') {
      /* Turn LF into CR + LF */
      if ((error = ConsPutChar(cons, '\r')))
        break;
    }
    if ((error = ConsPutChar(cons, ch)))
      break;
    req->wbuf++;
    req->left--;
  }
  xSemaphoreGive(tty->writeLock);

  return req->left < n ? 0 : error;
}

static inline int ConsGetChar(Device_t *cons, char *data) {
  return cons->ops->read(cons, &IOREQ_READ(0, data, 1));
}

static int TtyRead(Device_t *dev, IoReq_t *req) {
  TtyState_t *tty = dev->data;
  Device_t *cons = tty->cons;
  size_t n = req->left;
  int error;

  xSemaphoreTake(tty->readLock, portMAX_DELAY);
  while (req->left) {
    if ((error = ConsGetChar(cons, req->rbuf)))
      break;
    char ch = *req->rbuf++;
    req->left--;
    if (ch == '\n')
      break;
  }
  xSemaphoreGive(tty->readLock);

  return req->left < n ? 0 : error;
}
