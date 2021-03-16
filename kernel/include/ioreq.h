#pragma once

#include <sys/types.h>

/* Tracks progress of I/O operation. */
typedef struct IoReq {
  off_t offset;
  union {
    char *rbuf;
    const char *wbuf;
  };
  size_t left;
} IoReq_t;

#define IOREQ_READ(_offset, _buf, _len)                                        \
  (IoReq_t) {                                                                  \
    .offset = (_offset), .rbuf = (_buf), .left = (_len),                       \
  }

#define IOREQ_WRITE(_offset, _buf, _len)                                       \
  (IoReq_t) {                                                                  \
    .offset = (_offset), .wbuf = (_buf), .left = (_len),                       \
  }
