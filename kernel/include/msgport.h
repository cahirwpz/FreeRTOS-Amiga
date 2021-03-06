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

/* A message port stores at most one `Msg` message. The port must be associated
 * with a single `owner` task. Whenever a message arrives the task is notified
 * with NB_MSGPORT. A message occupies the port until it is replied. */
MsgPort_t *MsgPortCreate(TaskHandle_t owner);
void MsgPortDelete(MsgPort_t *mp);

/* Waits for `mp` to become empty.
 * Then it puts message and sends NB_MSGPORT `owner` task. */
void DoMsg(MsgPort_t *mp, Msg_t *msg);

/* Get message data is always non-blocking operation.
 * Only `mp` owner can fetch messages from a message port.
 * Returns data associated with message or NULL. */
void *GetMsgData(MsgPort_t *mp);

/* Empties `mp` and sends a NB_MSGPORT notification to the task that sent
 * the message. Clears out `Msg::task` when the message is replied. */
void ReplyMsg(MsgPort_t *mp);
