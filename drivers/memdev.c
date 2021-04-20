#include <string.h>
#include <devfile.h>
#include <ioreq.h>
#include <memdev.h>
#include <sys/errno.h>

static int MemoryOpen(DevFile_t *, FileFlags_t);
static int MemoryRead(DevFile_t *, IoReq_t *);

static DevFileOps_t MemoryOps = {
  .type = DT_MEM,
  .open = MemoryOpen,
  .close = NullDevClose,
  .read = MemoryRead,
  .write = NullDevWrite,
  .strategy = NullDevStrategy,
  .ioctl = NullDevIoctl,
  .event = NullDevEvent,
};

int AddMemoryDev(const char *name, const void *buf, size_t size) {
  DevFile_t *dev;
  int error;

  if ((error = AddDevFile(name, &MemoryOps, &dev)))
    return error;
  dev->size = size;
  dev->data = (void *)buf;
  return 0;
}

static int MemoryOpen(DevFile_t *dev __unused, FileFlags_t flags) {
  return (flags & F_WRITE) ? EACCES : 0;
}

static int MemoryRead(DevFile_t *dev, IoReq_t *req) {
  size_t n = req->left;
  if (req->offset + (ssize_t)req->left > dev->size)
    n = dev->size - req->offset;
  memcpy(req->rbuf, dev->data + req->offset, n);
  req->left -= n;
  return 0;
}
