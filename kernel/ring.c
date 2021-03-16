#include <ring.h>
#include <string.h>

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
  if (!RingFull(buf)) {
    buf->data[buf->head] = byte;
    RingProduce(buf, 1);
  }
}

int RingGetByte(Ring_t *buf) {
  if (RingEmpty(buf))
    return -1;
  uint8_t byte = buf->data[buf->tail];
  RingConsume(buf, 1);
  return byte;
}

size_t RingRead(Ring_t *buf, void *data, size_t len) {
  size_t done = 0;
  /* repeat when used space is split into two parts */
  while (done < len && !RingEmpty(buf)) {
    /* used space is either [tail, head) or [tail, size) */
    size_t size =
      (buf->tail < buf->head) ? buf->head - buf->tail : buf->size - buf->tail;
    if (size > len)
      size = len;
    memcpy(data + done, buf->data + buf->tail, size);
    RingConsume(buf, size);
    done += size;
  }
  return done;
}

size_t RingWrite(Ring_t *buf, const void *data, size_t len) {
  size_t done = 0;
  /* repeat when free space is split into two parts */
  while (done < len && !RingFull(buf)) {
    /* free space is either [head, tail) or [head, size) */
    size_t size =
      (buf->head < buf->tail) ? buf->tail - buf->head : buf->size - buf->head;
    if (size > len)
      size = len;
    memcpy(buf->data + buf->head, data + done, size);
    RingProduce(buf, size);
    done += size;
  }
  return done;
}
