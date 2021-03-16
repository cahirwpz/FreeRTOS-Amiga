#include <string.h>
#include <device.h>
#include <ioreq.h>
#include <memdev.h>

static int MemoryRead(Device_t *, IoReq_t *);

static DeviceOps_t MemoryOps = {.read = MemoryRead};

int AddMemoryDev(const char *name, const void *buf, size_t size) {
  Device_t *dev;
  int error;

  if ((error = AddDevice(name, &MemoryOps, &dev)))
    return error;
  dev->size = size;
  dev->data = (void *)buf;
  return 0;
}

static int MemoryRead(Device_t *dev, IoReq_t *req) {
  size_t n = req->left;
  if (req->offset + (ssize_t)req->left > dev->size)
    n = dev->size - req->offset;
  memcpy(req->rbuf, dev->data + req->offset, n);
  req->left -= n;
  return 0;
}
