#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/semphr.h>
#include <FreeRTOS/event_groups.h>

#include <buf.h>
#include <device.h>
#include <ioreq.h>
#include <libkern.h>

#define DEBUG 1
#include <debug.h>

/* Number of lists containing buffered blocks. */
#define NBUCKETS 16

#define BUCKET(dev, daddr) (((intptr_t)(dev) + (daddr)) % NBUCKETS)

static SemaphoreHandle_t BufLock;
static BufList_t Bucket[NBUCKETS]; /* all blocks with valid data */
static BufList_t LruLst;           /* free buffers with valid data */
static BufList_t EmptyLst;         /* free buffers that are empty */

void InitBufs(size_t nbufs) {
  BufLock = xSemaphoreCreateMutex();

  TAILQ_INIT(&LruLst);
  TAILQ_INIT(&EmptyLst);

  for (int i = 0; i < NBUCKETS; i++)
    TAILQ_INIT(&Bucket[i]);

  /* Initialize all buffers and put them on empty list. */
  Buf_t *bufs = kcalloc(nbufs, sizeof(Buf_t));
  for (size_t i = 0; i < nbufs; i++) {
    Buf_t *buf = &bufs[i];
    buf->waiters = xEventGroupCreate();
    TAILQ_INSERT_TAIL(&EmptyLst, buf, link);
  }
}

static Buf_t *AllocBuf(void) {
  Buf_t *buf = NULL;

  /* Initially every empty block is on empty list. */
  if (!TAILQ_EMPTY(&EmptyLst)) {
    buf = TAILQ_FIRST(&EmptyLst);
    TAILQ_REMOVE(&EmptyLst, buf, link);
    return buf;
  }

  DASSERT(!TAILQ_EMPTY(&LruLst));

  /* Eventually empty list will become exhausted.
   * Then we'll take the last recently used entry from LRU list. */
  buf = TAILQ_LAST(&LruLst, BufList);
  TAILQ_REMOVE(&LruLst, buf, link);
  BufList_t *bucket = &Bucket[BUCKET(buf->dev, buf->blkno)];
  TAILQ_REMOVE(bucket, buf, hash);
  return buf;
}

static int BufWait(Buf_t *buf) {
  int error = 0;

  taskENTER_CRITICAL();
  {
    while (buf->flags & B_BUSY)
      xEventGroupWaitBits(buf->waiters, B_BUSY, pdTRUE, pdFALSE, portMAX_DELAY);
    error = buf->error;
  }
  taskEXIT_CRITICAL();

  return error;
}

int BufRead(Device_t *dev, daddr_t blkno, Buf_t **bufp) {
  BufList_t *bucket = NULL;
  Buf_t *buf = NULL;
  int error = 0;

  xSemaphoreTake(BufLock, portMAX_DELAY);
  {
    bucket = &Bucket[BUCKET(dev, blkno)];

    /* Locate a valid block in the buffer. */
    TAILQ_FOREACH (buf, bucket, hash) {
      if (buf->dev == dev && buf->blkno == blkno) {
        taskENTER_CRITICAL();
        {
          if (buf->refcnt == 0)
            TAILQ_REMOVE(&LruLst, buf, link);
          buf->refcnt++;
        }
        taskEXIT_CRITICAL();

        xSemaphoreGive(BufLock);

        /* We could have found a buffer with pending I/O,
         * so wait for it to finish ! */
        error = BufWait(buf);
        goto done;
      }
    }

    buf = AllocBuf();
    buf->dev = dev;
    buf->blkno = blkno;
    buf->refcnt = 1;
    buf->error = 0;
    buf->flags |= B_BUSY;
    xEventGroupClearBits(buf->waiters, B_BUSY);
    TAILQ_INSERT_HEAD(bucket, buf, hash);
  }
  xSemaphoreGive(BufLock);

  IoReq_t req = IOREQ_READ(blkno * BLKSIZE, buf->data, BLKSIZE);
  error = dev->ops->read(dev, &req);

  taskENTER_CRITICAL();
  {
    buf->error = error;
    buf->flags &= ~B_BUSY;
    xEventGroupSetBits(buf->waiters, B_BUSY);
  }
  taskEXIT_CRITICAL();

done:
  *bufp = error ? NULL : buf;
  if (error)
    BufRelease(buf);
  return error;
}

/* Releases a buffer. If reference counter hits 0 the buffer can be reused to
 * cache another block. The buffer is then put at the beginning of LRU list. */
void BufRelease(Buf_t *buf) {
  xSemaphoreTake(BufLock, portMAX_DELAY);
  {
    taskENTER_CRITICAL();
    short refcnt = --buf->refcnt;
    taskEXIT_CRITICAL();

    if (refcnt == 0) {
      if (buf->error) {
        /* Reading into the buffer failed... so free the buffer. */
        TAILQ_INSERT_TAIL(&EmptyLst, buf, link);
        BufList_t *bucket = &Bucket[BUCKET(buf->dev, buf->blkno)];
        TAILQ_REMOVE(bucket, buf, hash);
      } else {
        /* The buffer has useful data so put it onto LRU list. */
        TAILQ_INSERT_HEAD(&LruLst, buf, link);
      }
    }
  }
  xSemaphoreGive(BufLock);
}
