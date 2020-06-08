#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/atomic.h>

#include <serial.h>
#include <file.h>

static long SerialRead(File_t *f, char *buf, size_t nbyte);
static long SerialWrite(File_t *f, const char *buf, size_t nbyte);
static void SerialClose(File_t *f);

static FileOps_t SerOps = {.read = (FileRead_t)SerialRead,
                           .write = (FileWrite_t)SerialWrite,
                           .close = (FileClose_t)SerialClose};

File_t *SerialOpen(unsigned baud) {
  static File_t f = {.ops = &SerOps};

  if (Atomic_Increment_u32(&f.usecount) == 0)
    SerialInit(baud);
  return &f;
}

static void SerialClose(File_t *f) {
  if (Atomic_Decrement_u32(&f->usecount) == 1)
    SerialKill();
}

static long SerialWrite(__unused File_t *f, const char *buf, size_t nbyte) {
  for (size_t i = 0; i < nbyte; i++)
    SerialPutChar(*buf++);
  return nbyte;
}

static long SerialRead(__unused File_t *f, char *buf, size_t nbyte) {
  size_t i = 0;
  while (i < nbyte) {
    buf[i] = SerialGetChar();
    if (buf[i++] == '\n')
      break;
  }
  return i;
}
