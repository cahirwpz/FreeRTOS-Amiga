#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/atomic.h>

#include <file.h>
#include <debug.h>
#include <memory.h>
#include <string.h>
#include <devfile.h>
#include <event.h>
#include <ioreq.h>
#include <sys/errno.h>

static int DevRead(File_t *, IoReq_t *);
static int DevWrite(File_t *, IoReq_t *);
static int DevIoctl(File_t *, u_long cmd, void *);
static int DevSeek(File_t *, long, int);
static int DevClose(File_t *);
static int DevEvent(File_t *, EvAction_t, EvFilter_t);

static FileOps_t DevFileOps = {
  .read = DevRead,
  .write = DevWrite,
  .ioctl = DevIoctl,
  .seek = DevSeek,
  .close = DevClose,
  .event = DevEvent,
};

static TAILQ_HEAD(, DevFile) DevFileList = TAILQ_HEAD_INITIALIZER(DevFileList);

DevFile_t *DevFileLookup(const char *name) {
  DevFile_t *dev = NULL;

  vTaskSuspendAll();

  TAILQ_FOREACH (dev, &DevFileList, node) {
    if (!strcmp(dev->name, name))
      break;
  }

  xTaskResumeAll();

  return dev;
}

int NullDevOk(void) {
  return 0;
}

__strong_alias(NullDevOpen, NullDevOk);
__strong_alias(NullDevClose, NullDevOk);

int NullDevNotImpl(void) {
  return ENOSYS;
}

__strong_alias(NullDevRead, NullDevNotImpl);
__strong_alias(NullDevWrite, NullDevNotImpl);
__strong_alias(NullDevStrategy, NullDevNotImpl);
__strong_alias(NullDevIoctl, NullDevNotImpl);
__strong_alias(NullDevEvent, NullDevNotImpl);

int AddDevFile(const char *name, DevFileOps_t *ops, DevFile_t **devp) {
  DevFile_t *dev;
  int error = 0;

  vTaskSuspendAll();

  if (DevFileLookup(name)) {
    error = EEXIST;
    goto leave;
  }

  if (!(dev = MemAlloc(sizeof(DevFile_t), 0))) {
    error = ENOMEM;
    goto leave;
  }

  TAILQ_INSERT_TAIL(&DevFileList, dev, node);
  dev->name = name;
  dev->ops = ops;
  dev->usecnt = 0;

  if (devp)
    *devp = dev;

  Log("Registered \'%s\' device file.\n", name);

leave:
  xTaskResumeAll();
  return error;
}

/* This routine should be a part of special file system. */
int OpenDevFile(const char *name, File_t *f) {
  DevFile_t *dev;
  int error = 0;

  vTaskSuspendAll();

  if (!(dev = DevFileLookup(name))) {
    error = ENOENT;
    goto leave;
  }

  if ((error = DevFileOpen(dev, f->flags)))
    goto leave;

  f->ops = &DevFileOps;
  f->type = FT_DEVICE;
  f->device = dev;
  dev->usecnt++;

leave:
  xTaskResumeAll();

  return error;
}

static int DevRead(File_t *f, IoReq_t *io) {
  DevFile_t *dev = f->device;
  long nbyte = io->left;
  int error = DevFileRead(dev, io);
  if ((dev->ops->type & DT_SEEKABLE) && !error)
    f->offset += nbyte - io->left;
  return error;
}

static int DevWrite(File_t *f, IoReq_t *io) {
  DevFile_t *dev = f->device;
  long nbyte = io->left;
  int error = DevFileWrite(dev, io);
  if ((dev->ops->type & DT_SEEKABLE) && !error)
    f->offset += nbyte - io->left;
  return error;
}

static int DevIoctl(File_t *f, u_long cmd, void *data) {
  DevFile_t *dev = f->device;
  return DevFileIoctl(dev, cmd, data, f->flags);
}

static int DevSeek(File_t *f, long offset, int whence) {
  DevFile_t *dev = f->device;

  if (!(dev->ops->type & DT_SEEKABLE))
    return ESPIPE;

  if (whence == SEEK_CUR) {
    offset += f->offset;
  } else if (whence == SEEK_END) {
    offset += dev->size;
  } else if (whence != SEEK_SET) {
    return EINVAL;
  }

  if (offset < 0) {
    offset = 0;
  } else if (offset > dev->size) {
    offset = dev->size;
  }

  f->offset = offset;
  return 0;
}

static int DevClose(File_t *f) {
  DevFile_t *dev = f->device;
  Atomic_Decrement_u32(&dev->usecnt);
  return DevFileClose(dev, f->flags);
}

static int DevEvent(File_t *f, EvAction_t act, EvFilter_t filt) {
  DevFile_t *dev = f->device;
  return DevFileEvent(dev, act, filt);
}
