#include <string.h>
#include <device.h>
#include <memdev.h>

static int MemoryRead(Device_t *, off_t, void *, size_t, ssize_t *);

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

static int MemoryRead(Device_t *dev, off_t offset, void *buf, size_t nbyte,
                      ssize_t *donep) {
  if (offset + (ssize_t)nbyte > dev->size)
    nbyte = dev->size - offset;
  memcpy(buf, dev->data + offset, nbyte);
  *donep = nbyte;
  return 0;
}
