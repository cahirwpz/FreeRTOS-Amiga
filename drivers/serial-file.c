#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/atomic.h>

#include <serial.h>
#include <file.h>
#include <sys/errno.h>

static int SerialRead(File_t *f, char *buf, size_t nbyte, long *donep);
static int SerialWrite(File_t *f, const char *buf, size_t nbyte, long *donep);
static int SerialClose(File_t *f);

static FileOps_t SerOps = {.read = (FileRead_t)SerialRead,
                           .write = (FileWrite_t)SerialWrite,
                           .close = (FileClose_t)SerialClose};

File_t *SerialOpen(unsigned baud) {
  static File_t f = {.ops = &SerOps, .readable = 1, .writeable = 1};

  uint32_t old = Atomic_Increment_u32(&f.usecount);
  configASSERT(old == 0);
  SerialInit(baud);
  return &f;
}

static int SerialClose(File_t *f) {
  if (f->usecount)
    return EBUSY;
  SerialKill();
  return 0;
}

static int SerialWrite(__unused File_t *f, const char *buf, size_t nbyte,
                       long *donep) {
  for (size_t i = 0; i < nbyte; i++)
    SerialPutChar(*buf++);
  *donep = nbyte;
  return 0;
}

static int SerialRead(__unused File_t *f, char *buf, size_t nbyte,
                      long *donep) {
  size_t i = 0;
  while (i < nbyte) {
    buf[i] = SerialGetChar();
    if (buf[i++] == '\n')
      break;
  }
  *donep = i;
  return 0;
}
