#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/atomic.h>

#include "tty.h"
#include "console.h"

static long TtyWrite(File_t *f, const char *buf, size_t nbyte);
static void TtyClose(File_t *f);

static FileOps_t TtyOps = {.write = (FileWrite_t)TtyWrite, .close = TtyClose};

File_t *TtyOpen(void) {
  static File_t f = {.ops = &TtyOps};
  Atomic_Increment_u32(&f.usecount);
  return &f;
}

static void TtyClose(File_t *f) {
  Atomic_Decrement_u32(&f->usecount);
}

static long TtyWrite(__unused File_t *f, const char *buf, size_t nbyte) {
  for (size_t i = 0; i < nbyte; i++)
    ConsolePutChar(*buf++);
  return nbyte;
}
