#pragma once

#include <sys/types.h>
#include <sys/queue.h>

typedef struct File File_t;
typedef struct IoReq IoReq_t;
typedef struct DevFile DevFile_t;
typedef struct DevFileOps DevFileOps_t;
typedef enum EvKind EvKind_t;

typedef int (*DevFileRead_t)(DevFile_t *dev, IoReq_t *req);
typedef int (*DevFileWrite_t)(DevFile_t *dev, IoReq_t *req);
typedef int (*DevFileIoctl_t)(DevFile_t *dev, u_long cmd, void *data);
typedef int (*DevFileEvent_t)(DevFile_t *dev, EvKind_t ev);

/* Operations available for a device file.
 * Simplified version of FreeBSD's cdevsw. */
struct DevFileOps {
  DevFileRead_t read;   /* read bytes from a device file */
  DevFileWrite_t write; /* write bytes to a device file */
  DevFileIoctl_t ioctl; /* read or modify device properties */
  DevFileEvent_t event; /* register handler for can-read or can-write events */
};

/* DevFile node needed by filesystem implementation.
 * Simplified version of FreeBSD's cdev. */
struct DevFile {
  TAILQ_ENTRY(DevFile) node; /* link on list of all device files */
  const char *name;
  DevFileOps_t *ops;
  void *data;      /* usually pointer to driver's private data */
  uint32_t usecnt; /* number of opened files referring to this device file */
  ssize_t size;    /* size in bytes, if `size` > 0 then device is seekable */
};

/* Add device file to global list of available devices.
 * Created device file is returned through `devp` pointer.
 *
 * Returns 0 on success, otherwise an errno code.  */
int AddDevFile(const char *name, DevFileOps_t *ops, DevFile_t **devp);

/* Finds a device file on the global list and attaches it to newly created file
 * object returned through `fp` pointer.
 *
 * Returns 0 on success, otherwise an errno code.  */
int OpenDevFile(const char *name, File_t **fp);
