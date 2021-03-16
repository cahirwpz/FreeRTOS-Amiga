#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <FreeRTOS/atomic.h>

#include <file.h>
#include <libkern.h>
#include <string.h>
#include <device.h>
#include <ioreq.h>
#include <sys/errno.h>

static int DevRead(File_t *, void *, size_t, long *);
static int DevWrite(File_t *, const void *, size_t, long *);
static int DevSeek(File_t *, long, int);
static int DevClose(File_t *);

static FileOps_t DevFileOps = {
  .read = DevRead,
  .write = DevWrite,
  .seek = DevSeek,
  .close = DevClose,
};

static TAILQ_HEAD(, Device) DeviceList = TAILQ_HEAD_INITIALIZER(DeviceList);

static Device_t *FindDevice(const char *name) {
  Device_t *dev;

  TAILQ_FOREACH (dev, &DeviceList, node) {
    if (!strcmp(dev->name, name))
      return dev;
  }

  return NULL;
}

int AddDevice(const char *name, DeviceOps_t *ops, Device_t **devp) {
  Device_t *dev;
  int error = 0;

  vTaskSuspendAll();

  if (FindDevice(name)) {
    error = EEXIST;
    goto leave;
  }

  if (!(dev = kmalloc(sizeof(Device_t)))) {
    error = ENOMEM;
    goto leave;
  }

  TAILQ_INSERT_TAIL(&DeviceList, dev, node);
  dev->name = name;
  dev->ops = ops;
  dev->usecnt = 0;

  if (devp)
    *devp = dev;

  kprintf("Registered \'%s\' device.\n", name);

leave:
  xTaskResumeAll();
  return error;
}

int OpenDevice(const char *name, File_t **fp) {
  Device_t *dev;
  File_t *f;
  int error = 0;

  vTaskSuspendAll();

  if (!(dev = FindDevice(name))) {
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

static int DevRead(File_t *f, void *buf, size_t len, long *donep) {
  DeviceRead_t read = f->device->ops->read;
  if (!read)
    return ENOSYS;

  IoReq_t req = IOREQ_READ(f->offset, buf, len);
  int error = read(f->device, &req);
  *donep = len - req.left;
  return error;
}

static int DevWrite(File_t *f, const void *buf, size_t len, long *donep) {
  DeviceWrite_t write = f->device->ops->write;
  if (!write)
    return ENOSYS;

  IoReq_t req = IOREQ_WRITE(f->offset, buf, len);
  int error = write(f->device, &req);
  *donep = len - req.left;
  return error;
}

static int DevSeek(File_t *f, long offset, int whence) {
  Device_t *dev = f->device;

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
