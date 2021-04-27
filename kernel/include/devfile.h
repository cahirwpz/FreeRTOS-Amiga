#pragma once

#include <sys/types.h>
#include <sys/queue.h>

typedef struct Buf Buf_t;
typedef struct File File_t;
typedef struct IoReq IoReq_t;
typedef struct DevFile DevFile_t;
typedef struct DevFileOps DevFileOps_t;
typedef enum EvAction EvAction_t;
typedef enum EvFilter EvFilter_t;
typedef enum FileFlags FileFlags_t;

/* Since device file is statless with regards to opened file interface we have
 * to drag `flags` as arguments. For `open` we need to know whether the device
 * can be opened with FREAD or FWRITE flags. If that succeeds identical flags
 * should be passed to `close`. We also need the flags with `ioctl` to check if
 * current file access permission allow performing the action (e.g. if file was
 * opened with FREAD we should reject _IOW commands. */
typedef int (*DevFileOpen_t)(DevFile_t *dev, FileFlags_t flags);
typedef int (*DevFileClose_t)(DevFile_t *dev, FileFlags_t flags);
typedef int (*DevFileRead_t)(DevFile_t *dev, IoReq_t *req);
typedef int (*DevFileWrite_t)(DevFile_t *dev, IoReq_t *req);
typedef int (*DevFileStrategy_t)(Buf_t *buf);
typedef int (*DevFileIoctl_t)(DevFile_t *dev, u_long cmd, void *data,
                              FileFlags_t flags);
typedef int (*DevFileEvent_t)(DevFile_t *dev, EvAction_t act, EvFilter_t filt);

/* Provide default implementation for given device file operation. */
int NullDevOpen(DevFile_t *dev, FileFlags_t flags);
int NullDevClose(DevFile_t *dev, FileFlags_t flags);
int NullDevRead(DevFile_t *dev, IoReq_t *req);
int NullDevWrite(DevFile_t *dev, IoReq_t *req);
int NullDevStrategy(Buf_t *buf);
int NullDevIoctl(DevFile_t *dev, u_long cmd, void *data, FileFlags_t flags);
int NullDevEvent(DevFile_t *dev, EvAction_t act, EvFilter_t filt);

typedef enum DevFileType {
  DT_OTHER = 0,       /* other non-seekable device file */
  DT_CONS = 1,        /* raw console file */
  DT_TTY = 2,         /* terminal file */
  DT_SEEKABLE = 0x80, /* other seekable device file (also a flag) */
  DT_DISK = 0x81,     /* disk device */
  DT_MEM = 0x82,      /* memory device */
} __packed DevFileType_t;

/* Operations available for a device file.
 * Simplified version of FreeBSD's cdevsw. */
struct DevFileOps {
  DevFileType_t type;
  DevFileOpen_t open;   /* check if device can be opened */
  DevFileClose_t close; /* called when file referring to the device is closed */
  DevFileRead_t read;   /* read bytes from a device file */
  DevFileWrite_t write; /* write bytes to a device file */
  DevFileStrategy_t strategy; /* perform block I/O operation */
  DevFileIoctl_t ioctl;       /* read or modify device properties */
  DevFileEvent_t event; /* register handler for can-read or can-write events */
};

#define DevFileOpen(_dev, _flags) ((_dev)->ops->open((_dev), (_flags)))
#define DevFileClose(_dev, _flags) ((_dev)->ops->close((_dev), (_flags)))
#define DevFileRead(_dev, _req) ((_dev)->ops->read((_dev), (_req)))
#define DevFileWrite(_dev, _req) ((_dev)->ops->write((_dev), (_req)))
#define DevFileIoctl(_dev, _cmd, _data, _flags)                                \
  ((_dev)->ops->ioctl((_dev), (_cmd), (_data), (_flags)))
#define DevFileEvent(_dev, _act, _filt)                                        \
  ((_dev)->ops->event((_dev), (_act), (_filt)))

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

/* Finds a device file named `name` on the global list, opens it and attaches
 * to empty `f` file object.
 *
 * Returns 0 on success, otherwise an errno code. */
int OpenDevFile(const char *name, File_t *f);

/* Looks up a device file named `name` ont the global list. */
DevFile_t *DevFileLookup(const char *name);
