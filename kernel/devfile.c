#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/atomic.h>

#include <file.h>
#include <libkern.h>
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
static int DevEvent(File_t *, EvKind_t);

static FileOps_t DevFileOps = {
  .read = DevRead,
  .write = DevWrite,
  .ioctl = DevIoctl,
  .seek = DevSeek,
  .close = DevClose,
  .event = DevEvent,
};

static TAILQ_HEAD(, DevFile) DevFileList = TAILQ_HEAD_INITIALIZER(DevFileList);

static DevFile_t *FindDevFile(const char *name) {
  DevFile_t *dev;

  TAILQ_FOREACH (dev, &DevFileList, node) {
    if (!strcmp(dev->name, name))
      break;
  }

  return dev;
}

int AddDevFile(const char *name, DevFileOps_t *ops, DevFile_t **devp) {
  DevFile_t *dev;
  int error = 0;

  vTaskSuspendAll();

  if (FindDevFile(name)) {
    error = EEXIST;
    goto leave;
  }

  if (!(dev = kmalloc(sizeof(DevFile_t)))) {
    error = ENOMEM;
    goto leave;
  }

  TAILQ_INSERT_TAIL(&DevFileList, dev, node);
  dev->name = name;
  dev->ops = ops;
  dev->usecnt = 0;

  if (devp)
    *devp = dev;

  klog("Registered \'%s\' device file.\n", name);

leave:
  xTaskResumeAll();
  return error;
}

int OpenDevFile(const char *name, File_t **fp) {
  DevFile_t *dev;
  File_t *f;
  int error = 0;

  vTaskSuspendAll();

  if (!(dev = FindDevFile(name))) {
    error = ENOENT;
    goto leave;
  }

  if (!(f = kcalloc(1, sizeof(File_t)))) {
    error = ENOMEM;
    goto leave;
  }

  f->ops = &DevFileOps;
  f->type = FT_DEVICE;
  f->device = dev;
  f->seekable = !!dev->size;
  dev->usecnt++;

  *fp = f;

leave:
  xTaskResumeAll();

  return error;
}

static int DevRead(File_t *f, IoReq_t *io) {
  DevFileRead_t read = f->device->ops->read;
  if (!read)
    return ENOSYS;
  return read(f->device, io);
}

static int DevWrite(File_t *f, IoReq_t *io) {
  DevFileWrite_t write = f->device->ops->write;
  if (!write)
    return ENOSYS;
  return write(f->device, io);
}

static int DevIoctl(File_t *f, u_long cmd, void *data) {
  DevFileIoctl_t ioctl = f->device->ops->ioctl;
  if (!ioctl)
    return ENOSYS;
  return ioctl(f->device, cmd, data);
}

static int DevSeek(File_t *f, long offset, int whence) {
  DevFile_t *dev = f->device;

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
  Atomic_Decrement_u32(&f->device->usecnt);
  return 0;
}

static int DevEvent(File_t *f, EvKind_t ev) {
  DevFileEvent_t event = f->device->ops->event;
  if (!event)
    return ENOSYS;
  return event(f->device, ev);
}
