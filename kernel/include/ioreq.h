#pragma once

#include <sys/types.h>

typedef struct tskTaskControlBlock *TaskHandle_t;

/* Tracks progress of I/O operation.
 *
 * `rbuf/wbuf` and `left` are updated during operation processing.
 * The request may be processed in another task, which will notify `origin`
 * when it finishes the job. */
typedef struct IoReq {
  off_t offset; /* valid only for seekable devices */
  union {
    char *rbuf;       /* valid if this is a read request */
    const char *wbuf; /* valid if this is a write request */
  };
  size_t left;
  uint8_t write : 1; /* is it read or write request ? */
  /* When `async` is set the operation returns EAGAIN instead of blocking,
   * a notification is sent to `origin` when operation can be safely resumed
   * without blocking. */
  uint8_t async : 1;

  /* This is the task that requested the I/O operation. */
  TaskHandle_t origin;
  /* If notification is sent to `origin` task, then `eNoAction` is selected
   * when `notifyBits` is zero, otherwise `eSetBits` action is selected and
   * `notifyBits` is passed as notification value. */
  uint32_t notifyBits;
  int error;
} IoReq_t;

#define IOREQ_READ(_offset, _buf, _len)                                        \
  (IoReq_t) {                                                                  \
    .offset = (_offset), .rbuf = (_buf), .left = (_len), .async = 0,           \
    .write = 0, .origin = xTaskGetCurrentTaskHandle(), .notifyBits = 0,        \
    .error = 0,                                                                \
  }

#define IOREQ_WRITE(_offset, _buf, _len)                                       \
  (IoReq_t) {                                                                  \
    .offset = (_offset), .wbuf = (_buf), .left = (_len), .async = 0,           \
    .write = 1, .origin = xTaskGetCurrentTaskHandle(), .notifyBits = 0,        \
    .error = 0,                                                                \
  }

#define IoReqNotify(req)                                                       \
  xTaskNotify((req)->origin, (req)->notifyBits,                                \
              (req)->notifyBits ? eSetBits : eNoAction)

#define IoReqNotifyFromISR(req)                                                \
  xTaskNotifyFromISR((req)->origin, (req)->notifyBits,                         \
                     (req)->notifyBits ? eSetBits : eNoAction,                 \
                     &xNeedRescheduleTask)

#define IoReqNotifyWait(req, valuep)                                           \
  xTaskNotifyWait(0, (req)->notifyBits, (valuep), portMAX_DELAY)
