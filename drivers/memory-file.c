#include <FreeRTOS/FreeRTOS.h>
#include <string.h>
#include <file.h>
#include <memory.h>
#include <sys/fcntl.h>
#include <sys/errno.h>

typedef struct MemFile {
  File_t f;
  const void *buf;
  long length;
} MemFile_t;

static int MemoryRead(MemFile_t *f, void *buf, size_t nbyte, long *donep);
static int MemorySeek(MemFile_t *f, long offset, int whence, long *newoffp);
static int MemoryClose(MemFile_t *f);

static FileOps_t MemOps = {.read = (FileRead_t)MemoryRead,
                           .seek = (FileSeek_t)MemorySeek,
                           .close = (FileClose_t)MemoryClose};

File_t *MemoryOpen(const void *buf, size_t length) {
  MemFile_t *mem = pvPortMalloc(sizeof(MemFile_t));
  mem->buf = buf;
  mem->length = length;
  mem->f.ops = &MemOps;
  mem->f.usecount = 1;
  mem->f.offset = 0;
  mem->f.readable = 1;
  mem->f.seekable = 1;
  return &mem->f;
}

static int MemoryClose(MemFile_t *mem) {
  if (--mem->f.usecount == 0)
    vPortFree(mem);
  return 0;
}

static int MemoryRead(MemFile_t *mem, void *buf, size_t nbyte, long *donep) {
  long start = mem->f.offset;
  long nread = nbyte;

  if (start + nread > mem->length)
    nread = mem->length - start;

  memcpy(buf, mem->buf + start, nread);
  mem->f.offset += nread;
  *donep = nread;
  return 0;
}

static int MemorySeek(MemFile_t *mem, long offset, int whence, long *newoffp) {
  if (whence == SEEK_CUR) {
    offset += mem->f.offset;
  } else if (whence == SEEK_END) {
    offset += mem->length;
  } else if (whence != SEEK_SET) {
    return EINVAL;
  }

  if (offset < 0) {
    offset = 0;
  } else if (offset > mem->length) {
    offset = mem->length;
  }

  mem->f.offset = offset;
  *newoffp = offset;
  return 0;
}
