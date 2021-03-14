#pragma once

#include <sys/types.h>

typedef struct File File_t;
typedef struct Inode Inode_t;
typedef struct Pipe Pipe_t;
typedef struct Device Device_t;

typedef int (*FileRead_t)(File_t *f, void *buf, size_t nbyte, long *donep);
typedef int (*FileWrite_t)(File_t *f, const void *buf, size_t nbyte,
                           long *donep);
typedef int (*FileSeek_t)(File_t *f, long offset, int whence);
typedef int (*FileClose_t)(File_t *f);

typedef struct FileOps {
  FileRead_t read;
  FileWrite_t write;
  FileSeek_t seek;
  FileClose_t close;
} FileOps_t;

typedef enum FileType {
  FT_INODE = 1,
  FT_DEVICE,
  FT_PIPE,
} __packed FileType_t;

typedef struct File {
  FileOps_t *ops;
  union {
    Device_t *device;
    Inode_t *inode;
    Pipe_t *pipe;
  };
  uint32_t usecount;
  off_t offset;
  FileType_t type;
  uint8_t readable : 1;
  uint8_t writable : 1;
  uint8_t seekable : 1;
} File_t;

/* Increase reference counter. */
File_t *FileHold(File_t *f);

/* Decrease reference counter. */
void FileDrop(File_t *f);

/* These behave like read/write/lseek known from UNIX */
int FileRead(File_t *f, void *buf, size_t nbyte, long *donep);
int FileWrite(File_t *f, const void *buf, size_t nbyte, long *donep);
int FileSeek(File_t *f, long offset, int whence);
int FileClose(File_t *f);
