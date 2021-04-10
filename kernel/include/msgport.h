#pragma once

#include <sys/types.h>

typedef struct tskTaskControlBlock *TaskHandle_t;
typedef struct MsgPort MsgPort_t;

typedef struct Msg {
  TaskHandle_t task; /* waits for the message to be replied, NULL when done */
  void *data;        /* data associated with the message */
} Msg_t;

MsgPort_t *MsgPortCreate(void);
void MsgPortDelete(MsgPort_t *mp);

void DoMsg(MsgPort_t *mp, void *data);
Msg_t *GetMsg(MsgPort_t *mp);
void ReplyMsg(Msg_t *msg);
