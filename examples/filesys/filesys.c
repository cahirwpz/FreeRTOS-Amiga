#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/queue.h>

#include <floppy.h>

#include "filesys.h"

typedef enum {
  FS_MOUNT,   /* mount filesystem */
  FS_UNMOUNT, /* unmount filesystem */
  FS_DIRENT,  /* fetch one directory entry */
  FS_OPEN,    /* open a file */
  FS_CLOSE,   /* close the file */
  FS_READ     /* read some bytes from the file */
} FsCmd_t;

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

typedef struct FsFile {
  File_t f;
  DirEntry_t *de;
} FsFile_t;

static long FsRead(FsFile_t *f, void *buf, size_t nbyte);
static long FsSeek(FsFile_t *f, long offset, int whence);
static void FsClose(FsFile_t *f);

__unused static FileOps_t FsOps = {.read = (FileRead_t)FsRead,
                                   .seek = (FileSeek_t)FsSeek,
                                   .close = (FileClose_t)FsClose};

static void vFileSysTask(__unused void *data) {
  for (;;) {
  }
}

bool FsMount(void) {
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

static TaskHandle_t filesys_handle;

#define FLOPPY_TASK_PRIO 3
#define FILESYS_TASK_PRIO 2

void FsInit(void) {
  FloppyInit(FLOPPY_TASK_PRIO);
  xTaskCreate(vFileSysTask, "filesys", configMINIMAL_STACK_SIZE, NULL,
              FILESYS_TASK_PRIO, &filesys_handle);
}
