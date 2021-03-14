#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/atomic.h>

#include <file.h>
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
  int error = f->ops->read(f, buf, nbyte, donep);
  if (!error && f->seekable)
    f->offset += *donep;
  return error;
}

int FileWrite(File_t *f, const void *buf, size_t nbyte, long *donep) {
  if (!f->ops->write)
    return ENOSYS;
  if (!f->writable)
    return EINVAL;
  int error = f->ops->write(f, buf, nbyte, donep);
  if (!error && f->seekable)
    f->offset += *donep;
  return error;
}

int FileSeek(File_t *f, long offset, int whence) {
  if (!f->seekable)
    return ESPIPE;
  if (!f->ops->seek)
    return ENOSYS;
  return f->ops->seek(f, offset, whence);
}

int FileClose(File_t *f) {
  if (!f->ops->close)
    return ENOSYS;
  if (Atomic_Decrement_u32(&f->usecount) > 1)
    return 0;
  return f->ops->close(f);
}
