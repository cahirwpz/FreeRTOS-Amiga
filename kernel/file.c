#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/atomic.h>

#include <event.h>
#include <ioreq.h>
#include <devfile.h>
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
  FileRdWr_t read = f->ops->read;
  if (!read)
    return ENOSYS;
  if (!f->readable)
    return EINVAL;
  IoReq_t io = IOREQ_READ(f->offset, buf, nbyte, f->nonblock);
  int error = read(f, &io);
  long done = nbyte - io.left;
  if (donep)
    *donep = done;
  if (!error && f->seekable)
    f->offset += done;
  return error;
}

int FileWrite(File_t *f, const void *buf, size_t nbyte, long *donep) {
  FileRdWr_t write = f->ops->write;
  if (!write)
    return ENOSYS;
  if (!f->writable)
    return EINVAL;
  IoReq_t io = IOREQ_WRITE(f->offset, buf, nbyte, f->nonblock);
  int error = write(f, &io);
  long done = nbyte - io.left;
  if (donep)
    *donep = done;
  if (!error && f->seekable)
    f->offset += *donep;
  return error;
}

int FileIoctl(File_t *f, u_long cmd, void *data) {
  if (!f->ops->ioctl)
    return ENOSYS;
  return f->ops->ioctl(f, cmd, data);
}

int FileSeek(File_t *f, long offset, int whence, long *newoffp) {
  if (!f->seekable)
    return ESPIPE;
  if (!f->ops->seek)
    return ENOSYS;
  int error = f->ops->seek(f, offset, whence);
  if (newoffp)
    *newoffp = f->offset;
  return error;
}

File_t *FileOpen(const char *name, int oflag) {
  File_t *f;

  if (OpenDevFile(name, &f))
    return NULL;

  int accmode = oflag & O_ACCMODE;
  if (accmode == O_RDONLY) {
    f->readable = 1;
  } else if (accmode == O_WRONLY) {
    f->writable = 1;
  } else if (accmode == O_RDWR) {
    f->readable = 1;
    f->writable = 1;
  }

  if (oflag & O_NONBLOCK)
    f->nonblock = 1;

  return f;
}

int FileClose(File_t *f) {
  if (!f->ops->close)
    return ENOSYS;
  if (Atomic_Decrement_u32(&f->usecount) > 1)
    return 0;
  return f->ops->close(f);
}

int FileEvent(File_t *f, EvKind_t ev) {
  FileEvent_t event = f->ops->event;
  if (!event)
    return ENOSYS;
  return event(f, ev);
}
