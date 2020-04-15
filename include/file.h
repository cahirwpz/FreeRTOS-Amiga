#ifndef _FILE_H_
#define _FILE_H_

#include <cdefs.h>
#include <stddef.h>

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

typedef struct File File_t;

typedef int (*FileRead_t)(File_t *f, void *buf, size_t nbyte);
typedef int (*FileWrite_t)(File_t *f, const void *buf, size_t nbyte);
typedef int (*FileSeek_t)(File_t *f, int offset, int whence);
typedef void (*FileClose_t)(File_t *f);

typedef struct {
  FileRead_t read;
  FileWrite_t write;
  FileSeek_t seek;
  FileClose_t close;
} FileOps_t;

typedef struct File {
  FileOps_t *ops;
  short usecount;
} File_t;

int FileRead(File_t *f, void *buf, size_t nbyte);
int FileWrite(File_t *f, const void *buf, size_t nbyte);
int FileSeek(File_t *f, long offset, int whence);
void FileClose(File_t *f);

void FilePutChar(File_t *f, char c);
void FilePrintf(File_t *f, const char *fmt, ...);

#endif /* !_FILE_H_ */
