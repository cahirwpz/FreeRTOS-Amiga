#include <serial.h>
#include <file.h>

static int SerialRead(File_t *f, char *buf, size_t nbyte);
static int SerialWrite(File_t *f, const char *buf, size_t nbyte);
static void SerialClose(File_t *f);

static FileOps_t SerOps = {
  .read = (FileRead_t)SerialRead,
  .write = (FileWrite_t)SerialWrite,
  .close = (FileClose_t)SerialClose
};

File_t *SerialOpen(unsigned baud) {
  static File_t f = {.ops = &SerOps};

  if (++f.usecount == 1)
    SerialInit(baud);
  return &f;
}

static void SerialClose(File_t *f) {
  if (--f->usecount == 0)
    SerialKill();
}

static int SerialWrite(__unused File_t *f, const char *buf, size_t nbyte) {
  for (size_t i = 0; i < nbyte; i++)
    SerialPutChar(*buf++);
  return nbyte;
}

static int SerialRead(__unused File_t *f, char *buf, size_t nbyte) {
  size_t i = 0;
  while (i < nbyte) {
    buf[i] = SerialGetChar();
    if (buf[i++] == '\n')
      break;
  }
  return i;
}
