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

/* Provide default implementation for given device file operation. */
static int NoDevOpen(DevFile_t *dev __unused, FileFlags_t flags __unused) {
  return 0;
}

static int NoDevClose(DevFile_t *dev __unused, FileFlags_t flags __unused) {
  return 0;
}

static int NoDevRead(DevFile_t *dev __unused, IoReq_t *req __unused) {
  return ENOSYS;
}

static int NoDevWrite(DevFile_t *dev __unused, IoReq_t *req __unused) {
  return ENOSYS;
}

static int NoDevStrategy(Buf_t *buf __unused) {
  return ENOSYS;
}

static int NoDevIoctl(DevFile_t *dev __unused, u_long cmd __unused,
                      void *data __unused, FileFlags_t flags __unused) {
  return ENOSYS;
}

static int NoDevEvent(DevFile_t *dev __unused, EvAction_t act __unused,
                      EvFilter_t filt __unused) {
  return ENOSYS;
}

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

  if (ops->open == NULL)
    ops->open = NoDevOpen;
  if (ops->close == NULL)
    ops->close = NoDevClose;
  if (ops->read == NULL)
    ops->read = NoDevRead;
  if (ops->write == NULL)
    ops->write = NoDevWrite;
  if (ops->ioctl == NULL)
    ops->ioctl = NoDevIoctl;
  if (ops->strategy == NULL)
    ops->strategy = NoDevStrategy;
  if (ops->event == NULL)
    ops->event = NoDevEvent;

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

  if ((error = dev->ops->open(dev, f->flags)))
    goto leave;

  f->ops = &DevFileOps;
  f->type = FT_DEVICE;
  f->device = dev;
  Atomic_Increment_u32(&dev->usecnt);

leave:
  xTaskResumeAll();

  return error;
}

static int DevRead(File_t *f, IoReq_t *io) {
  DevFile_t *dev = f->device;
  long nbyte = io->left;
  int error = dev->ops->read(dev, io);
  if ((dev->ops->type & DT_SEEKABLE) && !error)
    f->offset += nbyte - io->left;
  return error;
}

static int DevWrite(File_t *f, IoReq_t *io) {
  DevFile_t *dev = f->device;
  long nbyte = io->left;
  int error = dev->ops->write(dev, io);
  if ((dev->ops->type & DT_SEEKABLE) && !error)
    f->offset += nbyte - io->left;
  return error;
}

static int DevIoctl(File_t *f, u_long cmd, void *data) {
  DevFile_t *dev = f->device;
  return dev->ops->ioctl(dev, cmd, data, f->flags);
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
  return dev->ops->close(dev, f->flags);
}

static int DevEvent(File_t *f, EvAction_t act, EvFilter_t filt) {
  DevFile_t *dev = f->device;
  return dev->ops->event(dev, act, filt);
}
