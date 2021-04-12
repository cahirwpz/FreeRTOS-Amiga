#pragma once

#include <sys/types.h>

typedef struct tskTaskControlBlock *TaskHandle_t;
typedef struct MsgPort MsgPort_t;

typedef struct Msg {
  void *data;        /* data associated with the message */
  TaskHandle_t task; /* waits for the message to be replied, NULL when done */
} Msg_t;

#define MSG(_data)                                                             \
  (Msg_t) {                                                                    \
    .data = (_data), .task = NULL                                              \
  }

MsgPort_t *MsgPortCreate(void);
void MsgPortDelete(MsgPort_t *mp);

void DoMsg(MsgPort_t *mp, Msg_t *msg);
Msg_t *GetMsg(MsgPort_t *mp);
void ReplyMsg(Msg_t *msg);
