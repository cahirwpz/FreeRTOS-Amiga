#pragma once

#include <sys/types.h>
#include <sys/queue.h>

typedef struct File File_t;
typedef struct Device Device_t;
typedef struct DeviceOps DeviceOps_t;

typedef int (*DeviceRead_t)(Device_t *dev, off_t offset, void *buf, size_t len,
                            ssize_t *donep);
typedef int (*DeviceWrite_t)(Device_t *dev, off_t offset, const void *buf,
                             size_t len, ssize_t *donep);

struct DeviceOps {
  DeviceRead_t read;
  DeviceWrite_t write;
};

struct Device {
  TAILQ_ENTRY(Device) node;
  const char *name;
  DeviceOps_t *ops;
  void *data;
  uint32_t usecnt;
  ssize_t size; /* if size > 0 then device is seekable */
};

int AddDevice(const char *name, DeviceOps_t *ops, Device_t **devp);
int OpenDevice(const char *name, File_t **fp);
