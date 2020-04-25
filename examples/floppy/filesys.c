#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/queue.h>

#include <floppy.h>

#include "filesys.h"

typedef enum {
  FS_MOUNT,             /* mount filesystem */
  FS_UNMOUNT,           /* unmount filesystem */
  FS_DIRENT,            /* fetch one directory entry */
  FS_OPEN,              /* open a file */
  FS_CLOSE,             /* close the file */
  FS_READ               /* read some bytes from the file */
} FsCmd_t;

/* The type of message send to file system task. */
typedef struct FsMsg {
  FsCmd_t cmd;          /* request type */
  QueueHandle_t rq;     /* where to return a reply */
  union {               /* data specific to given request type */
    struct {} mount;
    struct {} umount;
    struct {} dirent;
    struct {} open;
    struct {} close;
    struct {} read;
  };
} FsMsg_t;

typedef struct FsFile {
  File_t f;
  DirEntry_t *de;
} FsFile_t;

static QueueHandle_t GetFsReplyQueue(void);

static long FsRead(FsFile_t *f, void *buf, size_t nbyte);
static long FsSeek(FsFile_t *f, long offset, int whence);
static void FsClose(FsFile_t *f);

__unused static FileOps_t FsOps = {
  .read = (FileRead_t)FsRead,
  .seek = (FileSeek_t)FsSeek,
  .close = (FileClose_t)FsClose
};

static void SendIO(FloppyIO_t *io, short track) {
  io->track = track;
  FloppySendIO(io);
}

static void WaitIO(QueueHandle_t replyQ, void *buf) {
  FloppyIO_t *io = NULL;
  (void)xQueueReceive(replyQ, &io, portMAX_DELAY);

  DiskSector_t *sectors[SECTOR_COUNT];
  DecodeTrack(io->buffer, sectors);
  for (int j = 0; j < SECTOR_COUNT; j++)
    DecodeSector(sectors[j], buf + j * SECTOR_SIZE);
}

static void vFileSysTask(__unused void *data) {
  QueueHandle_t replyQ = xQueueCreate(2, sizeof(FloppyIO_t *));
  void *buf = pvPortMalloc(SECTOR_SIZE * SECTOR_COUNT);

  /*
   * For double buffering we need (sic!) two track buffers:
   *  - one track will be owned by floppy driver
   *    which will set up a DMA transfer to it
   *  - the track will be decoded from MFM format
   *    and possibly read by this task
   */
  FloppyIO_t io[2];
  for (short i = 0; i < 2; i++) {
    io[i].cmd = CMD_READ;
    io[i].buffer = AllocTrack();
    io[i].replyQueue = replyQ;
  }

  for (;;) {
    short track = 0;
    short active = 0;

    /* Initiate double buffered reads. */
    SendIO(&io[active], track++);
    active ^= 1;

    do {
      /* Request asynchronous read into second buffer. */
      SendIO(&io[active], track++);
      active ^= 1;
      /* Wait for reply with first buffer and decode it. */
      WaitIO(replyQ, buf);
    } while (track < TRACK_COUNT);

    /* Finish last track. */
    WaitIO(replyQ, buf);

    /* Wait two seconds and repeat. */
    vTaskDelay(2000 / portTICK_PERIOD_MS);
  }
}

bool FsMount(void) {
  QueueHandle_t rq = GetFsReplyQueue();
  configASSERT(rq != NULL);

  return false;
}

/* Remember to free memory used up by a directory! */
int FsUnMount(void) {
  return 0;
}

const DirEntry_t *FsListDir(void **base_p) {
  (void)base_p;
  return NULL;
}

File_t *FsOpen(const char *name) {
  (void)name;
  return NULL;
}

static void FsClose(FsFile_t *ff) {
  (void)ff;
}

static long FsRead(FsFile_t *ff, void *buf, size_t nbyte) {
  (void)ff;
  (void)buf;
  (void)nbyte;
  return -1;
}

/* Does not involve direct interaction with the filesystem. */
static long FsSeek(FsFile_t *ff, long offset, int whence) {
  (void)ff;
  (void)offset;
  (void)whence;
  return -1;
}

static xTaskHandle filesys_handle;

#define FLOPPY_TASK_PRIO 3
#define FILESYS_TASK_PRIO 2

void FsInit(void) {
  FloppyInit(FLOPPY_TASK_PRIO);
  xTaskCreate(vFileSysTask, "filesys", configMINIMAL_STACK_SIZE, NULL,
              FILESYS_TASK_PRIO, &filesys_handle);
}

/* Use first pointer in thread local storage
 * as a reply queue for the filesystem */
static QueueHandle_t GetFsReplyQueue(void) {
	return pvTaskGetThreadLocalStoragePointer(xTaskGetCurrentTaskHandle(), 0);
}

static void SetFsReplyQueue(QueueHandle_t rq) {
  vTaskSetThreadLocalStoragePointer(xTaskGetCurrentTaskHandle(), 0, rq);
}

void CreateFsReplyQueue(void) {
  SetFsReplyQueue(xQueueCreate(1, sizeof(FsMsg_t)));
}

void DeleteFsReplyQueue(void) {
  vQueueDelete(GetFsReplyQueue());
  SetFsReplyQueue(NULL);
}
