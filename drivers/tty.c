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

#define BUFSIZ 256

/* Input buffer handles line editing. */
typedef struct InputQueue {
  char buf[BUFSIZ]; /* line begins at first byte */
  size_t len;       /* number of bytes in `buf` that are used */
  size_t eol;       /* bytes up to first occurence of end of line char */
  size_t done;      /* number of characters processed */
} InputQueue_t;

/* Output buffer state after character processing. */
typedef struct OutputQueue {
  char buf[BUFSIZ]; /* circular buffer data */
  size_t len;       /* number of bytes in `buf` that are used */
  size_t head;      /* index of first free byte in `buf` */
  size_t tail;      /* index of first valid byte in `buf` */
} OutputQueue_t;

typedef struct TtyState {
  TaskHandle_t task;
  const char *name;
  InputQueue_t *input;
  OutputQueue_t *output;
  /* Used for communication with file-like objects. */
  MsgPort_t *ctrlMp;
  MsgPort_t *readMp;
  MsgPort_t *writeMp;
  /* Used for communication with terminal device. */
  DevFile_t *cons;
  IoReq_t rxReq;
  IoReq_t txReq;
} TtyState_t;

static void TtyTask(void *);
static int TtyOpen(DevFile_t *, FileFlags_t);
static int TtyClose(DevFile_t *, FileFlags_t);
static int TtyRead(DevFile_t *, IoReq_t *);
static int TtyWrite(DevFile_t *, IoReq_t *);

static DevFileOps_t TtyOps = {
  .type = DT_TTY,
  .open = TtyOpen,
  .close = TtyClose,
  .read = TtyRead,
  .write = TtyWrite,
  .strategy = NullDevStrategy,
  .ioctl = NullDevIoctl,
  .event = NullDevEvent,
};

#define TTY_TASK_PRIO 2

int AddTtyDevFile(const char *name, DevFile_t *cons) {
  DevFile_t *dev;
  TtyState_t *tty;

  if (cons->ops->type != DT_CONS)
    return ENXIO;

  if (!(tty = MemAlloc(sizeof(TtyState_t), MF_ZERO)))
    return ENOMEM;

  tty->name = name;
  tty->cons = cons;

  int error;
  if ((error = AddDevFile(name, &TtyOps, &dev))) {
    MemFree(tty);
    return error;
  }

  dev->data = tty;

  return 0;
}

static int HandleRxReady(TtyState_t *tty) {
  DevFile_t *cons = tty->cons;
  IoReq_t *req = &tty->rxReq;
  InputQueue_t *input = tty->input;
  int error = 0;

  /* Append read characters at the end of line buffer. */
  while (input->len < BUFSIZ) {
    req->rbuf = input->buf + input->len;
    req->left = BUFSIZ - input->len;
    if ((error = DevFileRead(cons, req))) {
      DLOG("tty: rx-ready; would block\n");
      break;
    }

    /* Update line length. */
    input->len = BUFSIZ - req->left;
    DLOG("tty: rx-ready; input len %d\n", input->len);
  }

  return error;
}

static void HandleReadReq(TtyState_t *tty) {
  IoReq_t *req = GetMsgData(tty->readMp);
  if (req == NULL)
    return;

  InputQueue_t *input = tty->input;

  /* Copy up to line length or BUFSIZ if no EOL was found. */
  size_t n = (input->len == BUFSIZ) ? BUFSIZ : input->eol;

  if (n == 0)
    return;

  /* All characters typed in by the user must be processed before
   * they are removed from the line buffer. */
  if (n < input->done)
    return;

  if (req->left < n)
    n = req->left;

  /* Copy data into I/O request buffer. */
  memcpy(req->rbuf, input->buf, n);
  req->rbuf += n;
  req->left -= n;

  /* Update state of the line. */
  if (n < input->len)
    memmove(input->buf, input->buf + n, input->len - n);
  input->len -= n;
  input->eol -= n;
  input->done -= n;

  /* The request was handled, so return it to the owner. */
  ReplyMsg(tty->readMp);
  DLOG("tty: replied read request\n");
}

static int HandleTxReady(TtyState_t *tty) {
  DevFile_t *cons = tty->cons;
  IoReq_t *req = &tty->txReq;
  OutputQueue_t *output = tty->output;
  int error = 0;

  if (output->len > 0)
    DLOG("tty: tx-ready; output len %d\n", output->len);

  while (output->len > 0) {
    /* Used space is either [tail, head) or [tail, BUFSIZ) */
    size_t size = (output->tail < output->head) ? output->head - output->tail
                                                : BUFSIZ - output->tail;

    /* Send asynchronous write request. */
    req->wbuf = &output->buf[output->tail];
    req->left = size;
    if ((error = DevFileWrite(cons, req))) {
      DLOG("tty: tx-ready; would block\n");
      break;
    }

    /* Update tail position. */
    size_t done = size - req->left;
    output->tail += done;
    if (output->tail == BUFSIZ)
      output->tail = 0;
    output->len -= done;

    DLOG("tty: tx-ready; output len %d\n", output->len);
  }

  return error;
}

static void StoreChar(OutputQueue_t *output, uint8_t c) {
  DASSERT(output->len < BUFSIZ);
  output->buf[output->head++] = c;
  output->len++;
  if (output->head == BUFSIZ)
    output->head = 0;
}

static int HandleWriteReq(TtyState_t *tty) {
  IoReq_t *req = GetMsgData(tty->writeMp);
  if (req == NULL)
    return 0;

  OutputQueue_t *output = tty->output;

  while (req->left > 0) {
    uint8_t ch = *req->wbuf;
    if (ch == '\n') {
      /* Turn LF into CR + LF */
      if (output->len + 2 > BUFSIZ)
        return EAGAIN;
      StoreChar(output, '\r');
    } else {
      if (output->len + 1 > BUFSIZ)
        return EAGAIN;
    }
    StoreChar(output, ch);
    req->wbuf++;
    req->left--;
  }

  /* The request was handled, so return it to the owner. */
  ReplyMsg(tty->writeMp);
  DLOG("tty: replied write request\n");
  return 0;
}

/* Perform character echoing. */
static void ProcessInput(TtyState_t *tty) {
  InputQueue_t *input = tty->input;
  OutputQueue_t *output = tty->output;

  DASSERT(input->done <= input->len);

  while (input->done < input->len && output->len < BUFSIZ) {
    uint8_t *ch = (uint8_t *)&input->buf[input->done++];
    if (*ch == '\r') {
      /* Replace '\r' by '\n', but output '\r\n'. */
      *ch = '\n';
      StoreChar(output, '\r');
      StoreChar(output, '\n');
      input->eol = input->done;
    } else if (*ch < 32) {
      /* Translate control codes to ASCII characters prefixed with '^' */
      StoreChar(output, '^');
      StoreChar(output, *ch + 64);
    } else if (*ch == 127) {
      /* Translate DEL character to '^?'. */
      StoreChar(output, '^');
      StoreChar(output, '?');
    } else {
      /* Just echo the character. */
      StoreChar(output, *ch);
    }
  }
}

#define QUIT ((void *)-1UL)

static void TtyTask(void *data) {
  TtyState_t *tty = data;

  DevFileEvent(tty->cons, EV_ADD, EVFILT_READ);
  DevFileEvent(tty->cons, EV_ADD, EVFILT_WRITE);

  while (NotifyWait(NB_MSGPORT | NB_EVENT, portMAX_DELAY)) {
    DLOG("tty: wakeup\n");

    if (GetMsgData(tty->ctrlMp) == QUIT)
      break;

    HandleRxReady(tty);
    ProcessInput(tty);
    HandleReadReq(tty);
    HandleWriteReq(tty);
    HandleTxReady(tty);
    DLOG("tty: sleep\n");
  }

  DevFileEvent(tty->cons, EV_DELETE, EVFILT_READ);
  DevFileEvent(tty->cons, EV_DELETE, EVFILT_WRITE);

  /* Abort pending requests. */
  if (GetMsgData(tty->readMp))
    ReplyMsg(tty->readMp);
  if (GetMsgData(tty->writeMp))
    ReplyMsg(tty->writeMp);

  /* Unblock `TtyClose` and exit task. */
  ReplyMsg(tty->ctrlMp);
}

static int TtyOpen(DevFile_t *dev, FileFlags_t flags __unused) {
  if (!dev->usecnt) {
    TtyState_t *tty = dev->data;

    int error = DevFileOpen(tty->cons, F_READ | F_WRITE);
    if (error)
      return error;

    tty->input = MemAlloc(sizeof(InputQueue_t), MF_ZERO);
    tty->output = MemAlloc(sizeof(OutputQueue_t), MF_ZERO);

    xTaskCreate((TaskFunction_t)TtyTask, tty->name, configMINIMAL_STACK_SIZE,
                tty, TTY_TASK_PRIO, &tty->task);

    tty->ctrlMp = MsgPortCreate(tty->task);
    tty->readMp = MsgPortCreate(tty->task);
    tty->writeMp = MsgPortCreate(tty->task);

    tty->rxReq.flags = tty->txReq.flags = F_NONBLOCK;
  }

  return 0;
}

static int TtyClose(DevFile_t *dev, FileFlags_t flags __unused) {
  if (!dev->usecnt) {
    TtyState_t *tty = dev->data;

    /* Send quit request and wait for the thread to finish. */
    DoMsg(tty->ctrlMp, &MSG(QUIT));

    MsgPortDelete(tty->ctrlMp);
    MsgPortDelete(tty->readMp);
    MsgPortDelete(tty->writeMp);

    MemFree(tty->output);
    MemFree(tty->input);

    DevFileClose(tty->cons, F_READ | F_WRITE);
  }

  return ENOSYS;
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
