#include <file.h>

int FileRead(File_t *f, void *buf, size_t nbyte) {
  return f->ops->read(f, buf, nbyte);
}

int FileWrite(File_t *f, const void *buf, size_t nbyte) {
  return f->ops->write(f, buf, nbyte);
}

int FileSeek(File_t *f, long offset, int whence) {
  return f->ops->seek(f, offset, whence);
}

void FileClose(File_t *f) {
  f->ops->close(f);
}

void FilePutChar(File_t *f, char c) {
  FileWrite(f, &c, 1);
}
