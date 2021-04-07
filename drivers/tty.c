#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <tty.h>
#include <device.h>
#include <ioreq.h>
#include <libkern.h>
#include <string.h>
#include <sys/errno.h>

#define DEBUG 0
#include <debug.h>

/*
 * Terminal driver (tty) is described in: The Design of the UNIX Operating
 * System, Maurice J. Bach, p. 329.
 *
 * The driver connects up to serial port or graphical console and wraps it
 * with a line discipline. It's functions are:
 *  - to parse input strings into lines (line buffering),
 *  - to process "erase" character (backspace), "word erase" character (CTRL+W),
 *    "kill line" character (CTRL+U),
 *  - to echo received characters to the terminal,
 *  - to expand tab characters to a sequence of blank spaces,
 *  - to generate signals to processes when user enters control characters
 *    such as CTRL+C, CTRL+Z and so on,
 *  - to allow a raw mode that does not interpret special characters such as
 *    erase, kill or carriage return.
 *
 * TODO: Only basic line buffering and character processing are implemented.
 */

#define BUFSIZ 128

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
  /* Input buffer handles line editing. */
  struct {
    char buf[BUFSIZ]; /* line begins at first byte */
    size_t len;       /* number of bytes in `buf` that are used */
    size_t eol;       /* bytes up to first occurence of end of line char */
    size_t done;      /* number of characters processed */
  } input;
  /* Output buffer state after character processing. */
  struct {
    char buf[BUFSIZ]; /* circular buffer data */
    size_t len;       /* number of bytes in `buf` that are used */
    size_t head;      /* index of first free byte in `buf` */
    size_t tail;      /* index of first valid byte in `buf` */
  } output;
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

  tty->input.len = 0;
  tty->input.eol = 0;
  tty->input.done = 0;

  tty->output.len = 0;
  tty->output.head = 0;
  tty->output.tail = 0;

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

static int HandleRxReady(TtyState_t *tty) {
  Device_t *cons = tty->cons;
  IoReq_t *req = &tty->rxReq;
  int error;

  /* Append read characters at the end of line buffer. */
  while (tty->input.len < BUFSIZ) {
    req->rbuf = tty->input.buf + tty->input.len;
    req->left = BUFSIZ - tty->input.len;
    if ((error = cons->ops->read(cons, req)))
      return error;

    /* Update line length. */
    tty->input.len = BUFSIZ - req->left;
  }

  return 0;
}

static void HandleReadReq(TtyState_t *tty) {
  IoReq_t *req = tty->readReq;

  /* Copy up to line length or BUFSIZ if no EOL was found. */
  size_t n = (tty->input.len == BUFSIZ) ? BUFSIZ : tty->input.eol;

  if (n == 0)
    return;

  /* All characters typed in by the user must be processed before
   * they are removed from the line buffer. */
  if (n < tty->input.done)
    return;

  if (req->left < n)
    n = req->left;

  /* Copy data into I/O request buffer. */
  memcpy(req->rbuf, tty->input.buf, n);
  req->rbuf += n;
  req->left -= n;

  /* Update state of the line. */
  if (n < tty->input.len)
    memmove(tty->input.buf, tty->input.buf + n, tty->input.len - n);
  tty->input.len -= n;
  tty->input.eol -= n;
  tty->input.done -= n;

  /* The request was handled, so return it to the owner. */
  tty->readReq = NULL;
  IoReqNotify(req);
}

static int HandleTxReady(TtyState_t *tty) {
  Device_t *cons = tty->cons;
  IoReq_t *req = &tty->txReq;
  int error;

  while (tty->output.len > 0) {
    /* Used space is either [tail, head) or [tail, BUFSIZ) */
    size_t size = (tty->output.tail < tty->output.head)
                    ? tty->output.head - tty->output.tail
                    : BUFSIZ - tty->output.tail;

    /* Send asynchronous write request. */
    req->wbuf = &tty->output.buf[tty->output.tail];
    req->left = size;
    if ((error = cons->ops->write(cons, req)))
      return error;

    /* Update tail position. */
    size_t done = size - req->left;
    tty->output.tail += done;
    if (tty->output.tail == BUFSIZ)
      tty->output.tail = 0;
    tty->output.len -= done;
  }

  return 0;
}

static void StoreChar(TtyState_t *tty, uint8_t c) {
  DASSERT(tty->output.len < BUFSIZ);
  tty->output.buf[tty->output.head++] = c;
  tty->output.len++;
  if (tty->output.head == BUFSIZ)
    tty->output.head = 0;
}

static int HandleWriteReq(TtyState_t *tty) {
  IoReq_t *req = tty->writeReq;

  while (req->left > 0) {
    uint8_t ch = *req->wbuf;
    if (ch == '\n') {
      /* Turn LF into CR + LF */
      if (tty->output.len + 2 > BUFSIZ)
        return EAGAIN;
      StoreChar(tty, '\r');
    } else {
      if (tty->output.len + 1 > BUFSIZ)
        return EAGAIN;
    }
    StoreChar(tty, ch);
    req->wbuf++;
    req->left--;
  }

  /* The request was handled, so return it to the owner. */
  tty->writeReq = NULL;
  IoReqNotify(req);
  return 0;
}

/* Perform character echoing. */
static void ProcessInput(TtyState_t *tty) {
  DASSERT(tty->input.done <= tty->input.len);

  while (tty->input.done < tty->input.len && tty->output.len < BUFSIZ) {
    uint8_t *ch = (uint8_t *)&tty->input.buf[tty->input.done++];
    if (*ch == '\r') {
      /* Replace '\r' by '\n', but output '\r\n'. */
      *ch = '\n';
      StoreChar(tty, '\r');
      StoreChar(tty, '\n');
      tty->input.eol = tty->input.done;
    } else if (*ch < 32) {
      /* Translate control codes to ASCII characters prefixed with '^' */
      StoreChar(tty, '^');
      StoreChar(tty, *ch + 64);
    } else if (*ch == 127) {
      /* Translate DEL character to '^?'. */
      StoreChar(tty, '^');
      StoreChar(tty, '?');
    } else {
      /* Just echo the character. */
      StoreChar(tty, *ch);
    }
  }
}

static void TtyTask(void *data) {
  TtyState_t *tty = data;
  uint32_t value;

  do {
    int error;

    HandleRxReady(tty);
    ProcessInput(tty);
    if (tty->readReq)
      HandleReadReq(tty);

    /* Retry if terminal tx buffer was full but device tx buffer was not. */
    do {
      error = tty->writeReq ? HandleWriteReq(tty) : 0;
    } while (!HandleTxReady(tty) && (error == EAGAIN));
  } while (xTaskNotifyWait(0, INPUTQ | OUTPUTQ, &value, portMAX_DELAY));
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
