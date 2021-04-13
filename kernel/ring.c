#include <ioreq.h>
#include <ring.h>
#include <string.h>
#include <libkern.h>

static inline void RingProduce(Ring_t *buf, size_t len) {
  buf->count += len;
  buf->head += len;
  if (buf->head == buf->size)
    buf->head = 0;
}

static inline void RingConsume(Ring_t *buf, size_t len) {
  buf->count -= len;
  buf->tail += len;
  if (buf->tail == buf->size)
    buf->tail = 0;
}

void RingPutByte(Ring_t *buf, uint8_t byte) {
  buf->data[buf->head] = byte;
  RingProduce(buf, 1);
}

uint8_t RingGetByte(Ring_t *buf) {
  uint8_t byte = buf->data[buf->tail];
  RingConsume(buf, 1);
  return byte;
}

void RingRead(Ring_t *buf, IoReq_t *req) {
  /* repeat when used space is split into two parts */
  while (req->left && !RingEmpty(buf)) {
    /* used space is either [tail, head) or [tail, size) */
    size_t size =
      (buf->tail < buf->head) ? buf->head - buf->tail : buf->size - buf->tail;
    if (size > req->left)
      size = req->left;
    memcpy(req->rbuf, buf->data + buf->tail, size);
    req->rbuf += size;
    req->left -= size;
    RingConsume(buf, size);
  }
}

void RingWrite(Ring_t *buf, IoReq_t *req) {
  /* repeat when free space is split into two parts */
  while (req->left && !RingFull(buf)) {
    /* free space is either [head, tail) or [head, size) */
    size_t size =
      (buf->head < buf->tail) ? buf->tail - buf->head : buf->size - buf->head;
    if (size > req->left)
      size = req->left;
    memcpy(buf->data + buf->head, req->wbuf, size);
    req->wbuf += size;
    req->left -= size;
    RingProduce(buf, size);
  }
}

Ring_t *RingAlloc(size_t size) {
  Ring_t *buf = kmalloc(sizeof(Ring_t) + size);
  buf->head = 0;
  buf->tail = 0;
  buf->count = 0;
  buf->size = size;
  return buf;
}
