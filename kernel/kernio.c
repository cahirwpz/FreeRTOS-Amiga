#include <libkern.h>
#include <stdarg.h>
#include <stdio.h>
#include <device.h>
#include <file.h>

File_t *kopen(const char *name, int oflag) {
  File_t *f;

  if (OpenDevice(name, &f))
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

  return f;
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
  return FileSeek(f, offset, whence) ? -1 : f->offset;
}
