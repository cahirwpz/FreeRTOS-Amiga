#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/atomic.h>

#include <stdarg.h>
#include <libkern.h>
#include <file.h>
#include <stdio.h>
#include <sys/errno.h>

File_t *FileHold(File_t *f) {
  uint32_t old = Atomic_Increment_u32(&f->usecount);
  configASSERT(old > 0);
  return f;
}

void FileDrop(File_t *f) {
  Atomic_Decrement_u32(&f->usecount);
}

int FileRead(File_t *f, void *buf, size_t nbyte, long *donep) {
  if (!f->ops->read)
    return ENOSYS;
  if (!f->readable)
    return EINVAL;
  return f->ops->read(f, buf, nbyte, donep);
}

int FileWrite(File_t *f, const void *buf, size_t nbyte, long *donep) {
  if (!f->ops->write)
    return ENOSYS;
  if (!f->writeable)
    return EINVAL;
  return f->ops->write(f, buf, nbyte, donep);
}

int FileSeek(File_t *f, long offset, int whence, long *newoffp) {
  if (!f->ops->seek)
    return ENOSYS;
  if (!f->seekable)
    return ESPIPE;
  return f->ops->seek(f, offset, whence, newoffp);
}

int FileClose(File_t *f) {
  if (!f->ops->close)
    return ENOSYS;
  if (Atomic_Decrement_u32(&f->usecount) > 1)
    return 0;
  return f->ops->close(f);
}

void kfputchar(File_t *f, char c) {
  long r;
  FileWrite(f, &c, 1, &r);
}

void kfprintf(File_t *f, const char *fmt, ...) {
  void PutChar(char c) {
    kfputchar(f, c);
  }

  va_list ap;

  va_start(ap, fmt);
  kvprintf(PutChar, fmt, ap);
  va_end(ap);
}

long kfread(File_t *f, void *buf, size_t nbyte) {
  long done;
  return FileRead(f, buf, nbyte, &done) ? -1 : done;
}

long kfwrite(File_t *f, const void *buf, size_t nbyte) {
  long done;
  return FileWrite(f, buf, nbyte, &done) ? -1 : done;
}

long kfseek(File_t *f, long offset, int whence) {
  long newoff;
  return FileSeek(f, offset, whence, &newoff) ? -1 : newoff;
}
