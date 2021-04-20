#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/queue.h>

#include <input.h>
#include <ioreq.h>
#include <sys/errno.h>

#define DEBUG 0
#include <debug.h>

#define EVENTQUEUE_SIZE 128

QueueHandle_t InputEventQueueCreate(void) {
  return xQueueCreate(EVENTQUEUE_SIZE, sizeof(InputEvent_t));
}

void InputEventQueueDelete(QueueHandle_t q) {
  vQueueDelete(q);
}

int InputEventInjectFromISR(QueueHandle_t q, const InputEvent_t *iev,
                            size_t n) {
  size_t avail = EVENTQUEUE_SIZE - uxQueueMessagesWaitingFromISR(q);

  DASSERT(n > 0);

  if (avail < n)
    return ENOSPC;

  do {
    xQueueSendFromISR(q, iev++, &xNeedRescheduleTask);
  } while (--n);

  return 0;
}

int InputEventRead(QueueHandle_t q, IoReq_t *io) {
  size_t done = 0;

  if (io->left < sizeof(InputEvent_t))
    return EINVAL;

  do {
    if (xQueueReceive(q, io->rbuf,
                      (io->flags & F_NONBLOCK) ? 0 : portMAX_DELAY)) {
      io->left -= sizeof(InputEvent_t);
      io->rbuf += sizeof(InputEvent_t);
      done++;
    } else if (done) {
      return 0;
    } else if (io->flags & F_NONBLOCK) {
      return EAGAIN;
    }
  } while (io->left >= sizeof(InputEvent_t));

  return 0;
}
