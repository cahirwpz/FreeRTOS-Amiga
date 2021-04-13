#pragma once

#include <sys/types.h>
#include <sys/fcntl.h>

typedef struct File File_t;
typedef struct Inode Inode_t;
typedef struct IoReq IoReq_t;
typedef struct Pipe Pipe_t;
typedef struct DevFile DevFile_t;
typedef enum EvKind EvKind_t;

typedef int (*FileRdWr_t)(File_t *f, IoReq_t *io);
typedef int (*FileIoctl_t)(File_t *f, u_long cmd, void *data);
typedef int (*FileSeek_t)(File_t *f, long offset, int whence);
typedef int (*FileEvent_t)(File_t *f, EvKind_t ev);
typedef int (*FileClose_t)(File_t *f);

/* Operations available for a file object.
 * Simplified version of FreeBSD's fileops. */
typedef struct FileOps {
  FileRdWr_t read;   /* read bytes from a file */
  FileRdWr_t write;  /* write bytes to a file */
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
    DevFile_t *device;
    Inode_t *inode;
    Pipe_t *pipe;
  };
  uint32_t usecount; /* number of file desciptors referring to this file */
  off_t offset;      /* cursor position for seekable files */
  FileType_t type;
  uint8_t readable : 1; /* set if call to FileOps::read is allowed */
  uint8_t writable : 1; /* set if call to FileOps::write is allowed */
  uint8_t seekable : 1; /* set if call to FileOps::seek is allowed */
  uint8_t nonblock : 1; /* set read & write return EAGAIN instead blocking */
} File_t;

/* Increase reference counter. */
File_t *FileHold(File_t *f);

/* Decrease reference counter. */
void FileDrop(File_t *f);

/* These behave like read/write/lseek known from UNIX */
File_t *FileOpen(const char *name, int oflag);
int FileRead(File_t *f, void *buf, size_t nbyte, long *donep);
int FileWrite(File_t *f, const void *buf, size_t nbyte, long *donep);
int FileIoctl(File_t *f, u_long cmd, void *data);
int FileSeek(File_t *f, long offset, int whence, long *newoffp);
int FileClose(File_t *f);

void FilePrintf(File_t *f, const char *fmt, ...);
void FileHexDump(File_t *f, void *ptr, size_t length);

/* Registers calling task to be notified with NB_EVENT
 * when can-read or can-write event happens on the file. */
int FileEvent(File_t *f, EvKind_t ev);
