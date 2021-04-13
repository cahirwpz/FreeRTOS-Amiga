#pragma once

#include <sys/types.h>
#include <sys/queue.h>

typedef struct File File_t;
typedef struct IoReq IoReq_t;
typedef struct Device Device_t;
typedef struct DeviceOps DeviceOps_t;
typedef enum EvKind EvKind_t;

typedef int (*DeviceRead_t)(Device_t *dev, IoReq_t *req);
typedef int (*DeviceWrite_t)(Device_t *dev, IoReq_t *req);
typedef int (*DeviceIoctl_t)(Device_t *dev, u_long cmd, void *data);
typedef int (*DeviceEvent_t)(Device_t *dev, EvKind_t ev);

struct DeviceOps {
  DeviceRead_t read;
  DeviceWrite_t write;
  DeviceIoctl_t ioctl;
  DeviceEvent_t event;
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

Device_t *AddDeviceAux(const char *name, DeviceOps_t *ops, void *data);

int DeviceEvent(Device_t *dev, EvKind_t ev);
