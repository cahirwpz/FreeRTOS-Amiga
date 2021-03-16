#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <tty.h>
#include <device.h>
#include <libkern.h>
#include <sys/errno.h>

/* TODO Implement rudimentary canonical mode, i.e. character echoing,
 * rewriting '\n' to '\r\n' on output, line buffering and character deletion. */

#define LINEBUF_SIZE 256

typedef struct TtyState {
  Device_t *cons;
  SemaphoreHandle_t writeLock;
  SemaphoreHandle_t readLock;
  char line[LINEBUF_SIZE];
  size_t lineLength;
} TtyState_t;

static int TtyRead(Device_t *, off_t, void *, size_t, ssize_t *);
static int TtyWrite(Device_t *, off_t, const void *, size_t, ssize_t *);

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
  ts->lineLength = 0;

  if ((error = AddDevice(name, &TtyOps, &dev)))
    return error;
  dev->data = (void *)ts;
  return 0;
}

static inline int ConsWrite(Device_t *cons, const char *data, size_t len) {
  return cons->ops->write(cons, 0, data, len, NULL);
}

static int TtyWrite(Device_t *dev, off_t offset __unused, const void *data,
                    size_t len, ssize_t *donep) {
  TtyState_t *tty = dev->data;
  Device_t *cons = tty->cons;
  const char *buf = data;
  size_t done;
  int error;

  xSemaphoreTake(tty->writeLock, portMAX_DELAY);
  for (done = 0; done < len; done++) {
    char ch = buf[done];
    if (ch == '\n') {
      /* Turn LF into CR + LF */
      if ((error = ConsWrite(cons, "\r\n", 2)))
        break;
    } else {
      /* Pass through all other characters. */
      if ((error = ConsWrite(cons, buf + done, 1)))
        break;
    }
  }
  xSemaphoreGive(tty->writeLock);

  if (donep)
    *donep = done;
  return done ? 0 : error;
}

static inline int ConsGetChar(Device_t *cons, char *data) {
  return cons->ops->read(cons, 0, data, 1, NULL);
}

static int TtyRead(Device_t *dev, off_t offset __unused, void *data, size_t len,
                   ssize_t *donep) {
  TtyState_t *tty = dev->data;
  Device_t *cons = tty->cons;
  char *buf = data;
  size_t done = 0;
  int error;

  xSemaphoreTake(tty->readLock, portMAX_DELAY);
  while (done < len) {
    if ((error = ConsGetChar(cons, buf + done)))
      break;
    if (buf[done++] == '\n')
      break;
  }
  xSemaphoreGive(tty->readLock);

  if (donep)
    *donep = done;
  return done ? 0 : error;
}
