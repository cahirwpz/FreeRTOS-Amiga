#pragma once

#include <sys/types.h>

typedef struct IoReq IoReq_t;

typedef struct Ring {
  size_t head;    /* producing data moves head forward */
  size_t tail;    /* consuming data moves tail forward */
  size_t count;   /* number of bytes currently stored in the buffer */
  size_t size;    /* total size of the buffer */
  uint8_t data[]; /* buffer that stores data */
} Ring_t;

static inline bool RingEmpty(Ring_t *buf) {
  return buf->count == 0;
}

static inline bool RingFull(Ring_t *buf) {
  return buf->count == buf->size;
}

/* Put `byte` into `buf`. You MUST check if `buf` is non-full before! */
void RingPutByte(Ring_t *buf, uint8_t byte);

/* Put `byte` into `buf`. You MUST check if `buf` is non-empty before! */
uint8_t RingGetByte(Ring_t *buf);

/* Transfer data from `req` into `buf`. Up to `req::left` bytes will be
 * transferred, but no more than `buf::size` - `buf::count`. */
void RingRead(Ring_t *buf, IoReq_t *req);

/* Transfer data from `buf` into `req`. Up to `req::left` bytes will be
 * transferred, but no more than `buf::count`. */
void RingWrite(Ring_t *buf, IoReq_t *req);

/* Allocate and initialize a ring buffer of `size` bytes. */
Ring_t *RingAlloc(size_t size);
