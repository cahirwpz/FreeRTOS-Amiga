#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/queue.h>

#include <floppy.h>
#include <sys/errno.h>

#include "filesys.h"

typedef enum {
  FS_MOUNT,   /* mount filesystem */
  FS_UNMOUNT, /* unmount filesystem */
  FS_DIRENT,  /* fetch one directory entry */
  FS_OPEN,    /* open a file */
  FS_CLOSE,   /* close the file */
  FS_READ     /* read some bytes from the file */
} FsCmd_t;

typedef struct FsFile {
  File_t f;
  DirEntry_t *de;
} FsFile_t;

/* The type of message send to file system task. */
typedef struct FsMsg {
  FsCmd_t cmd; /* request type */
  union {      /* data specific to given request type */
    struct {
    } mount;
    struct {
    } umount;
    struct {
    } dirent;
    struct {
    } open;
    struct {
    } close;
    struct {
    } read;
  } request;
  struct {
    long *replyp;      /* store result here before task wakeup */
    TaskHandle_t task; /* notify this task when reponse is ready */
  } response;
} FsMsg_t;

static int FsRead(FsFile_t *f, void *buf, size_t nbyte, long *donep);
static int FsSeek(FsFile_t *f, long offset, int whence, long *newoffp);
static int FsClose(FsFile_t *f);

static FileOps_t FsOps = {.read = (FileRead_t)FsRead,
                          .seek = (FileSeek_t)FsSeek,
                          .close = (FileClose_t)FsClose};

static void vFileSysTask(__unused void *data) {
  for (;;) {
  }
}

static long FsSendMsg(FsMsg_t *msg) {
  /* TODO: fill in missing 'msg' fields, send it to filesystem task and wait for
   * the response. */
  (void)msg;
  return 0;
}

bool FsMount(void) {
  FsMsg_t msg = {.cmd = FS_MOUNT};
  return FsSendMsg(&msg);
}

/* Remember to free memory used up by a directory! */
int FsUnMount(void) {
  FsMsg_t msg = {.cmd = FS_UNMOUNT};
  return FsSendMsg(&msg);
}

const DirEntry_t *FsListDir(void **base_p) {
  FsMsg_t msg = {.cmd = FS_DIRENT};
  FsSendMsg(&msg);
  return *base_p;
}

File_t *FsOpen(const char *name) {
  FsMsg_t msg = {.cmd = FS_OPEN};
  (void)name;
  return (File_t *)FsSendMsg(&msg);
}

static int FsClose(FsFile_t *ff) {
  FsMsg_t msg = {.cmd = FS_CLOSE};
  (void)ff;
  return FsSendMsg(&msg);
}

static int FsRead(FsFile_t *ff, void *buf, size_t nbyte, long *donep) {
  FsMsg_t msg = {.cmd = FS_READ};
  (void)ff, (void)buf, (void)nbyte, (void)donep;
  return FsSendMsg(&msg);
}

/* Does not involve direct interaction with the filesystem. */
static int FsSeek(FsFile_t *ff, long offset, int whence, long *newoffp) {
  (void)ff, (void)offset, (void)whence, (void)newoffp;
  return ENOSYS;
}

static TaskHandle_t filesysHandle;

#define FLOPPY_TASK_PRIO 3
#define FILESYS_TASK_PRIO 2

void FsInit(void) {
  (void)FsOps;

  FloppyInit(FLOPPY_TASK_PRIO);
  xTaskCreate(vFileSysTask, "filesys", configMINIMAL_STACK_SIZE, NULL,
              FILESYS_TASK_PRIO, &filesysHandle);
}
