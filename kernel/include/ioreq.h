#pragma once

#include <sys/types.h>

/* Tracks progress of I/O operation.
 *
 * `rbuf/wbuf` and `left` are updated during operation processing. */
typedef struct IoReq {
  off_t offset; /* valid only for seekable devices */
  union {
    char *rbuf;       /* valid if this is a read request */
    const char *wbuf; /* valid if this is a write request */
  };
  size_t left;
  uint8_t write : 1; /* is it read or write request ? */
  /* When `nonblock` is set the operation returns EAGAIN instead of blocking. */
  uint8_t nonblock : 1;
  int error;
} IoReq_t;

#define _IOREQ_READ(_off, _buf, _len, _nb)                                     \
  (IoReq_t) {                                                                  \
    .offset = (_off), .rbuf = (char *)(_buf), .left = (_len),                  \
    .nonblock = (_nb), .write = 0, .error = 0                                  \
  }

#define _IOREQ_WRITE(_off, _buf, _len, _nb)                                    \
  (IoReq_t) {                                                                  \
    .offset = (_off), .wbuf = (const char *)(_buf), .left = (_len),            \
    .nonblock = (_nb), .write = 1, .error = 0                                  \
  }

#define IOREQ_READ(_off, _buf, _len) _IOREQ_READ(_off, _buf, _len, 0)
#define IOREQ_WRITE(_off, _buf, _len) _IOREQ_WRITE(_off, _buf, _len, 0)
#define IOREQ_READ_NB(_off, _buf, _len) _IOREQ_READ(_off, _buf, _len, 1)
#define IOREQ_WRITE_NB(_off, _buf, _len) _IOREQ_WRITE(_off, _buf, _len, 1)
