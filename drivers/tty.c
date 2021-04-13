#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>

#include <tty.h>
#include <devfile.h>
#include <file.h>
#include <msgport.h>
#include <event.h>
#include <notify.h>
#include <ioreq.h>
#include <memory.h>
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
  File_t *cons;
  /* Used for communication with file-like objects. */
  Msg_t *readMsg;
  MsgPort_t *readMp;
  Msg_t *writeMsg;
  MsgPort_t *writeMp;
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
static int TtyRead(DevFile_t *, IoReq_t *);
static int TtyWrite(DevFile_t *, IoReq_t *);

static DevFileOps_t TtyOps = {
  .read = TtyRead,
  .write = TtyWrite,
};

#define TTY_TASK_PRIO 2

int AddTtyDevFile(const char *name, File_t *cons) {
  DevFile_t *dev;
  TtyState_t *tty;
  int error;

  if (!(tty = MemAlloc(sizeof(TtyState_t), 0)))
    return ENOMEM;
  tty->cons = cons;
  tty->cons->nonblock = 1;

  tty->input.len = 0;
  tty->input.eol = 0;
  tty->input.done = 0;

  tty->output.len = 0;
  tty->output.head = 0;
  tty->output.tail = 0;

  if ((error = AddDevFile(name, &TtyOps, &dev)))
    return error;
  dev->data = (void *)tty;

  xTaskCreate((TaskFunction_t)TtyTask, name, configMINIMAL_STACK_SIZE, tty,
              TTY_TASK_PRIO, &tty->task);

  tty->readMp = MsgPortCreate(tty->task);
  tty->writeMp = MsgPortCreate(tty->task);

  tty->rxReq.nonblock = tty->txReq.nonblock = 1;
  return 0;
}

static int HandleRxReady(TtyState_t *tty) {
  DevFile_t *cons = tty->cons->device;
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
  IoReq_t *req = tty->readMsg->data;

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
  ReplyMsg(tty->readMsg);
  tty->readMsg = NULL;
}

static int HandleTxReady(TtyState_t *tty) {
  DevFile_t *cons = tty->cons->device;
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
  IoReq_t *req = tty->writeMsg->data;

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
  ReplyMsg(tty->writeMsg);
  tty->writeMsg = NULL;
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

  (void)FileEvent(tty->cons, EV_READ);
  (void)FileEvent(tty->cons, EV_WRITE);

  while (NotifyWait(NB_MSGPORT | NB_EVENT, portMAX_DELAY)) {
    if (!tty->readMsg)
      tty->readMsg = GetMsg(tty->readMp);

    if (!tty->writeMsg)
      tty->writeMsg = GetMsg(tty->writeMp);

    HandleRxReady(tty);
    ProcessInput(tty);
    if (tty->readMsg)
      HandleReadReq(tty);
    if (tty->writeMsg)
      HandleWriteReq(tty);
    HandleTxReady(tty);
  }
}

static int TtyRead(DevFile_t *dev, IoReq_t *req) {
  TtyState_t *tty = dev->data;
  size_t n = req->left;

  DoMsg(tty->readMp, &MSG(req));

  return req->left < n ? 0 : req->error;
}

static int TtyWrite(DevFile_t *dev, IoReq_t *req) {
  TtyState_t *tty = dev->data;
  size_t n = req->left;

  DoMsg(tty->writeMp, &MSG(req));

  return req->left < n ? 0 : req->error;
}
