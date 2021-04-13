#pragma once

#include <sys/types.h>

typedef struct File File_t;
typedef struct Inode Inode_t;
typedef struct Pipe Pipe_t;
typedef struct Device Device_t;
typedef enum EvKind EvKind_t;

typedef int (*FileRead_t)(File_t *f, void *buf, size_t nbyte, long *donep);
typedef int (*FileWrite_t)(File_t *f, const void *buf, size_t nbyte,
                           long *donep);
typedef int (*FileIoctl_t)(File_t *f, u_long cmd, void *data);
typedef int (*FileSeek_t)(File_t *f, long offset, int whence);
typedef int (*FileEvent_t)(File_t *f, EvKind_t ev);
typedef int (*FileClose_t)(File_t *f);

/* Operations available for a file object.
 * Simplified version of FreeBSD's fileops. */
typedef struct FileOps {
  FileRead_t read;   /* read bytes from a file */
  FileWrite_t write; /* write bytes to a file */
  FileIoctl_t ioctl; /* read or modify file properties (if applicable) */
  FileSeek_t seek;   /* move cursor position (if applicable) */
  FileClose_t close; /* free up resources */
  FileEvent_t event; /* register handler for can-read or can-write events */
} FileOps_t;

typedef enum FileType {
  FT_INODE = 1,  /* i-node in disk filesystem */
  FT_DEVICE = 2, /* device file */
  FT_PIPE = 3,   /* pipe object */
} __packed FileType_t;

/* File object that can be associated with a file decriptor.
 * Simplified version of FreeBSD's file. */
typedef struct File {
  FileOps_t *ops;
  union {
    Device_t *device;
    Inode_t *inode;
    Pipe_t *pipe;
  };
  uint32_t usecount; /* number of file desciptors referring to this file */
  off_t offset;      /* cursor position for seekable files */
  FileType_t type;
  uint8_t readable : 1; /* set if call to FileOps::read is allowed */
  uint8_t writable : 1; /* set if call to FileOps::write is allowed */
  uint8_t seekable : 1; /* set if call to FileOps::seek is allowed */
} File_t;

/* Increase reference counter. */
File_t *FileHold(File_t *f);

/* Decrease reference counter. */
void FileDrop(File_t *f);

/* These behave like read/write/lseek known from UNIX */
int FileRead(File_t *f, void *buf, size_t nbyte, long *donep);
int FileWrite(File_t *f, const void *buf, size_t nbyte, long *donep);
int FileIoctl(File_t *f, u_long cmd, void *data);
int FileSeek(File_t *f, long offset, int whence);
int FileClose(File_t *f);

/* Registers calling task to be notified with NB_EVENT
 * when can-read or can-write event happens on the device. */
int FileEvent(File_t *f, EvKind_t ev);
