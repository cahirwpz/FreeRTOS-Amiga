#pragma once

#include <sys/queue.h>
#include <sys/types.h>

/* Simplifying assumption: all buffered blocks have 512 bytes. */
#define BLKSIZE 512

typedef enum BufFlags {
  B_BUSY = 1, /* I/O in progress */
} __packed BufFlags_t;

struct EventGroupDef_t;
typedef struct EventGroupDef_t *EventGroupHandle_t;
typedef struct DevFile DevFile_t;

/* Structure that is used to manage buffer of a single block.
 *
 * (!) modify or wait for state change with interrupts disabled
 * (b) modify while holding BufLock mutex
 * (+) safe for use by API user
 */
typedef struct Buf {
  TAILQ_ENTRY(Buf) hash;           /* (b) link on fast lookup list */
  TAILQ_ENTRY(Buf) link;           /* (b) link on Empty / LRU list */
  EventGroupHandle_t waiters;      /* (!) tasks waiting for I/O to complete */
  DevFile_t *dev;                  /* (b) block belongs to this device */
  daddr_t blkno;                   /* (b) block address on the block device */
  short refcnt;                    /* (!) if zero then block can be reused */
  short error;                     /* (!) set with errno code if I/O failed */
  BufFlags_t flags;                /* (!) B_* flags or'ed together */
  char data[BLKSIZE] __aligned(4); /* (+) raw data fetched from the device */
} Buf_t;

typedef TAILQ_HEAD(BufList, Buf) BufList_t;

void InitBuf(size_t nbufs);
int BufRead(DevFile_t *dev, daddr_t blkno, Buf_t **bufp);
void BufRelease(Buf_t *buf);
