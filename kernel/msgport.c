#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/queue.h>

#include <msgport.h>
#include <libkern.h>
#include <sys/errno.h>

#define DEBUG 1
#include <debug.h>

struct MsgPort {
  TaskHandle_t owner;
  QueueHandle_t queue;
};

MsgPort_t *MsgPortCreate(void) {
  MsgPort_t *mp = kcalloc(1, sizeof(MsgPort_t));
  mp->owner = xTaskGetCurrentTaskHandle();
  if (mp->owner == NULL)
    portPANIC();
  mp->queue = xQueueCreate(1, sizeof(Msg_t *));
  return mp;
}

void MsgPortDelete(MsgPort_t *mp) {
  vQueueDelete(mp->queue);
  kfree(mp);
}

void DoMsg(MsgPort_t *mp, void *data) {
  Msg_t msg = {.task = xTaskGetCurrentTaskHandle(), .data = data};
  Msg_t *msgp = &msg;
  xQueueSend(mp->queue, &msgp, portMAX_DELAY);
  xTaskNotify(mp->owner, 0, eNoAction);
  DPRINTF("DoMsg: Wakeup owner %x!\n", mp->owner);
  xTaskNotifyWait(0, 0, NULL, portMAX_DELAY);
}

Msg_t *GetMsg(MsgPort_t *mp) {
  Msg_t *msg = NULL;
  xQueueReceive(mp->queue, &msg, 0);
  return msg;
}

void ReplyMsg(Msg_t *msg) {
  xTaskNotify(msg->task, 0, eNoAction);
  msg->task = NULL;
}
