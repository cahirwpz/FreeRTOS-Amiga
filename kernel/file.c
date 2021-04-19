#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/atomic.h>

#include <event.h>
#include <ioreq.h>
#include <devfile.h>
#include <file.h>
#include <sys/errno.h>

int NullFileOk(void) {
  return 0;
}

__strong_alias(NullFileClose, NullFileOk);

int NullFileNotImpl(void) {
  return ENOSYS;
}

__strong_alias(NullFileRead, NullFileNotImpl);
__strong_alias(NullFileWrite, NullFileNotImpl);
__strong_alias(NullFileIoctl, NullFileNotImpl);
__strong_alias(NullFileEvent, NullFileNotImpl);

int NullFileNotSeekable(void) {
  return ESPIPE;
}

__strong_alias(NullFileSeek, NullFileNotSeekable);

File_t *FileHold(File_t *f) {
  uint32_t old = Atomic_Increment_u32(&f->usecount);
  configASSERT(old > 0);
  return f;
}

void FileDrop(File_t *f) {
  Atomic_Decrement_u32(&f->usecount);
}

int FileRead(File_t *f, void *buf, size_t nbyte, long *donep) {
  if (!(f->flags & F_READ))
    return EINVAL;
  IoReq_t io = IOREQ_READ(f->offset, buf, nbyte, f->flags & F_IOFLAGS);
  int error = f->ops->read(f, &io);
  if (donep)
    *donep = nbyte - io.left;
  return error;
}

int FileWrite(File_t *f, const void *buf, size_t nbyte, long *donep) {
  if (!(f->flags & F_WRITE))
    return EINVAL;
  IoReq_t io = IOREQ_WRITE(f->offset, buf, nbyte, f->flags & F_IOFLAGS);
  int error = f->ops->write(f, &io);
  if (donep)
    *donep = nbyte - io.left;
  return error;
}

int FileIoctl(File_t *f, u_long cmd, void *data) {
  return f->ops->ioctl(f, cmd, data);
}

int FileSeek(File_t *f, long offset, int whence, long *newoffp) {
  int error = f->ops->seek(f, offset, whence);
  if (newoffp)
    *newoffp = f->offset;
  return error;
}

File_t *FileOpen(const char *name, int oflags) {
  File_t *f;

  if (OpenDevFile(name, oflags, &f))
    return NULL;

  return f;
}

int FileClose(File_t *f) {
  if (Atomic_Decrement_u32(&f->usecount) > 1)
    return 0;
  return f->ops->close(f);
}

int FileEvent(File_t *f, EvKind_t ev) {
  return f->ops->event(f, ev);
}
