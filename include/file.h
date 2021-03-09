#ifndef _FILE_H_
#define _FILE_H_

#include <sys/types.h>

typedef struct File File_t;

typedef long (*FileRead_t)(File_t *f, void *buf, size_t nbyte);
typedef long (*FileWrite_t)(File_t *f, const void *buf, size_t nbyte);
typedef long (*FileSeek_t)(File_t *f, long offset, int whence);
typedef void (*FileClose_t)(File_t *f);

typedef struct {
  FileRead_t read;
  FileWrite_t write;
  FileSeek_t seek;
  FileClose_t close;
} FileOps_t;

typedef struct File {
  FileOps_t *ops;
  uint32_t usecount;
  off_t offset;
} File_t;

/* Increase reference counter. */
File_t *FileHold(File_t *f);

/* These behave like read/write/lseek known from UNIX */
long FileRead(File_t *f, void *buf, size_t nbyte);
long FileWrite(File_t *f, const void *buf, size_t nbyte);
long FileSeek(File_t *f, long offset, int whence);
void FileClose(File_t *f);

void FilePutChar(File_t *f, char c);
void FilePrintf(File_t *f, const char *fmt, ...);
void FileHexDump(File_t *f, void *ptr, size_t length);

File_t *MemoryOpen(const void *buf, size_t nbyte);

#endif /* !_FILE_H_ */
