#pragma once

#include <sys/types.h>

typedef struct IoReq IoReq_t;

typedef struct Ring {
  size_t head;    /*!< producing data moves head forward */
  size_t tail;    /*!< consuming data moves tail forward */
  size_t count;   /*!< number of bytes currently stored in the buffer */
  size_t size;    /*!< total size of the buffer */
  uint8_t data[]; /*!< buffer that stores data */
} Ring_t;

static inline bool RingEmpty(Ring_t *buf) {
  return buf->count == 0;
}

static inline bool RingFull(Ring_t *buf) {
  return buf->count == buf->size;
}

void RingPutByte(Ring_t *buf, uint8_t byte);
int RingGetByte(Ring_t *buf);

void RingRead(Ring_t *buf, IoReq_t *req);
void RingWrite(Ring_t *buf, IoReq_t *req);

Ring_t *RingAlloc(size_t size);
