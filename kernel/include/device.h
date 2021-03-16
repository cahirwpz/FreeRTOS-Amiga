#pragma once

#include <sys/types.h>
#include <sys/queue.h>

typedef struct File File_t;
typedef struct IoReq IoReq_t;
typedef struct Device Device_t;
typedef struct DeviceOps DeviceOps_t;

typedef int (*DeviceRead_t)(Device_t *dev, IoReq_t *req);
typedef int (*DeviceWrite_t)(Device_t *dev, IoReq_t *req);

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