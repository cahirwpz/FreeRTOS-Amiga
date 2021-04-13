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

/* Operations available for a device file.
 * Simplified version of FreeBSD's cdevsw. */
struct DeviceOps {
  DeviceRead_t read;   /* read bytes from a device file */
  DeviceWrite_t write; /* write bytes to a device file */
  DeviceIoctl_t ioctl; /* read or modify device properties */
  DeviceEvent_t event; /* register handler for can-read or can-write events */
};

/* Device node needed by filesystem implementation.
 * Simplified version of FreeBSD's cdev. */
struct Device {
  TAILQ_ENTRY(Device) node; /* link on list of all device files */
  const char *name;
  DeviceOps_t *ops;
  void *data;      /* usually pointer to driver's private data */
  uint32_t usecnt; /* number of opened files referring to this device */
  ssize_t size;    /* size in bytes, if `size` > 0 then device is seekable */
};

/* Add device file to global list of available devices.
 * Created device is returned through `devp` pointer.
 *
 * Returns 0 on success, otherwise an errno code.  */
int AddDevice(const char *name, DeviceOps_t *ops, Device_t **devp);

/* Simplified version of `AddDevice`. Sets `Device::data` to `data`. */
Device_t *AddDeviceAux(const char *name, DeviceOps_t *ops, void *data);

/* Finds a device on the global list and attaches it to newly created file
 * object returned through `fp` pointer.
 *
 * Returns 0 on success, otherwise an errno code.  */
int OpenDevice(const char *name, File_t **fp);

/* Registers calling task to be notified with NB_EVENT
 * when can-read or can-write event happens on the device. */
int DeviceEvent(Device_t *dev, EvKind_t ev);
