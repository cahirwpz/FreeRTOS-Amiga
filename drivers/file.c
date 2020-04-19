#include <stdarg.h>
#include <stdio.h>
#include <file.h>

long FileRead(File_t *f, void *buf, size_t nbyte) {
  return f->ops->read ? f->ops->read(f, buf, nbyte) : -1;
}

long FileWrite(File_t *f, const void *buf, size_t nbyte) {
  return f->ops->write ? f->ops->write(f, buf, nbyte) : -1;
}

long FileSeek(File_t *f, long offset, int whence) {
  return f->ops->seek ? f->ops->seek(f, offset, whence) : -1;
}

void FileClose(File_t *f) {
  if (f->ops->close)
    f->ops->close(f);
}

void FilePutChar(File_t *f, char c) {
  FileWrite(f, &c, 1);
}

void FilePrintf(File_t *f, const char *fmt, ...) {
  void PutChar(char c) {
    FileWrite(f, &c, 1);
  }

  va_list ap;

  va_start(ap, fmt);
  kvprintf(PutChar, fmt, ap);
  va_end(ap);
}
