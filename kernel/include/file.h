#pragma once

#include <sys/types.h>

typedef struct File File_t;

typedef int (*FileRead_t)(File_t *f, void *buf, size_t nbyte, long *donep);
typedef int (*FileWrite_t)(File_t *f, const void *buf, size_t nbyte,
                           long *donep);
typedef int (*FileSeek_t)(File_t *f, long offset, int whence, long *newoffp);
typedef int (*FileClose_t)(File_t *f);

typedef struct FileOps {
  FileRead_t read;
  FileWrite_t write;
  FileSeek_t seek;
  FileClose_t close;
} FileOps_t;

typedef enum FileType {
  FT_INODE = 1,
  FT_BLKDEV,
  FT_CHRDEV,
  FT_PIPE,
} FileType_t;

typedef struct File {
  FileOps_t *ops;
  uint32_t usecount;
  off_t offset;
  uint8_t readable : 1;
  uint8_t writeable : 1;
  uint8_t seekable : 1;
} File_t;

/* Increase reference counter. */
File_t *FileHold(File_t *f);

/* Decrease reference counter. */
void FileDrop(File_t *f);

/* These behave like read/write/lseek known from UNIX */
int FileRead(File_t *f, void *buf, size_t nbyte, long *donep);
int FileWrite(File_t *f, const void *buf, size_t nbyte, long *donep);
int FileSeek(File_t *f, long offset, int whence, long *newoffp);
int FileClose(File_t *f);
