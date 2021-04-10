#include <msgport.h>

MsgPort_t *MsgPortCreate(void) {
  return NULL;
}

void MsgPortDelete(MsgPort_t *mp) {
  (void)mp;
}

void DoMsg(MsgPort_t *mp, void *data) {
  (void)mp, (void)data;
}

Msg_t *GetMsg(MsgPort_t *mp) {
  (void)mp;
  return NULL;
}

void ReplyMsg(Msg_t *msg) {
  (void)msg;
}
