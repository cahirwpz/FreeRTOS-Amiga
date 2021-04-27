#pragma once

#include <sys/types.h>
#include <file.h>

/* Tracks progress of I/O operation on a file object.
 *
 * `rbuf/wbuf` and `left` are updated during operation processing. */
typedef struct IoReq {
  off_t offset; /* valid only for seekable devices */
  union {
    char *rbuf;       /* valid if this is a read request */
    const char *wbuf; /* valid if this is a write request */
  };
  size_t left;
  FileFlags_t flags; /* currently only F_NONBLOCK is supported */
  uint8_t write : 1; /* is it read or write request ? */
  int error;
} IoReq_t;

#define IOREQ_READ(_off, _buf, _len, _iof)                                     \
  (IoReq_t) {                                                                  \
    .offset = (_off), .rbuf = (char *)(_buf), .left = (_len), .flags = (_iof), \
    .write = 0, .error = 0                                                     \
  }

#define IOREQ_WRITE(_off, _buf, _len, _iof)                                    \
  (IoReq_t) {                                                                  \
    .offset = (_off), .wbuf = (const char *)(_buf), .left = (_len),            \
    .flags = (_iof), .write = 1, .error = 0                                    \
  }
