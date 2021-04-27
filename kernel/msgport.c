#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/queue.h>

#include <msgport.h>
#include <notify.h>
#include <memory.h>
#include <sys/errno.h>

#define DEBUG 0
#include <debug.h>

struct MsgPort {
  TaskHandle_t owner;
  QueueHandle_t queue;
};

MsgPort_t *MsgPortCreate(TaskHandle_t owner) {
  MsgPort_t *mp = MemAlloc(sizeof(MsgPort_t), MF_ZERO);
  Assert(owner != NULL);
  mp->owner = owner;
  mp->queue = xQueueCreate(1, sizeof(Msg_t *));
  return mp;
}

void MsgPortDelete(MsgPort_t *mp) {
  vQueueDelete(mp->queue);
  MemFree(mp);
}

void DoMsg(MsgPort_t *mp, Msg_t *msg) {
  msg->task = xTaskGetCurrentTaskHandle();
  DLOG("DoMsg: send message %x!\n", msg);
  xQueueSend(mp->queue, &msg, portMAX_DELAY);
  NotifySend(mp->owner, NB_MSGPORT);
  DLOG("DoMsg: wakeup owner %x!\n", mp->owner);
  (void)NotifyWait(NB_MSGPORT, portMAX_DELAY);
}

void *GetMsgData(MsgPort_t *mp) {
  Msg_t *msg = NULL;
  xQueuePeek(mp->queue, &msg, 0);
  if (msg == NULL)
    return NULL;
  DLOG("GetMsg: message at %x!\n", msg);
  return msg->data;
}

void ReplyMsg(MsgPort_t *mp) {
  Msg_t *msg;
  vTaskSuspendAll();
  DLOG("ReplyMsg: wakeup sender %x!\n", msg->task);
  xQueueReceive(mp->queue, &msg, 0);
  Assert(msg != NULL);
  NotifySend(msg->task, NB_MSGPORT);
  msg->task = NULL;
  xTaskResumeAll();
}
