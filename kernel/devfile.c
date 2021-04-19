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

  if (FindDevFile(name)) {
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

int OpenDevFile(const char *name, int oflags, File_t **fp) {
  DevFile_t *dev;
  File_t *f;
  int error = 0;
  FileFlags_t flags;

  accmode_t accmode = oflags & O_ACCMODE;
  if (accmode == O_RDONLY)
    flags = F_READ;
  else if (accmode == O_WRONLY)
    flags = F_WRITE;
  else if (accmode == O_RDWR)
    flags = F_READ | F_WRITE;
  else
    return EINVAL;

  flags |= (oflags & O_NONBLOCK) ? F_NONBLOCK : 0;

  vTaskSuspendAll();

  if (!(dev = FindDevFile(name))) {
    error = ENOENT;
    goto leave;
  }

  if ((error = dev->ops->open(dev, flags)))
    goto leave;

  if (!(f = MemAlloc(sizeof(File_t), MF_ZERO))) {
    error = ENOMEM;
    goto leave;
  }

  f->ops = &DevFileOps;
  f->type = FT_DEVICE;
  f->device = dev;
  dev->usecnt++;

  *fp = f;

leave:
  xTaskResumeAll();

  return error;
}

static int DevRead(File_t *f, IoReq_t *io) {
  DevFile_t *dev = f->device;
  long nbyte = io->left;
  int error = dev->ops->read(dev, io);
  if (dev->ops->seekable && !error)
    f->offset += nbyte - io->left;
  return error;
}

static int DevWrite(File_t *f, IoReq_t *io) {
  DevFile_t *dev = f->device;
  long nbyte = io->left;
  int error = dev->ops->write(dev, io);
  if (dev->ops->seekable && !error)
    f->offset += nbyte - io->left;
  return error;
}

static int DevIoctl(File_t *f, u_long cmd, void *data) {
  DevFile_t *dev = f->device;
  return dev->ops->ioctl(dev, cmd, data, f->flags);
}

static int DevSeek(File_t *f, long offset, int whence) {
  DevFile_t *dev = f->device;

  if (!dev->ops->seekable)
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
  if (Atomic_Decrement_u32(&dev->usecnt) > 1)
    return 0;
  return dev->ops->close(dev, f->flags);
}

static int DevEvent(File_t *f, EvKind_t ev) {
  DevFile_t *dev = f->device;
  return dev->ops->event(dev, ev);
}
