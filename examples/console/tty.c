#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/atomic.h>

#include "tty.h"
#include "console.h"

static int TtyWrite(File_t *f, const char *buf, size_t nbyte, long *donep);
static int TtyClose(File_t *f);

static FileOps_t TtyOps = {.write = (FileWrite_t)TtyWrite, .close = TtyClose};

File_t *TtyOpen(void) {
  static File_t f = {.ops = &TtyOps, .writable = 1};
  Atomic_Increment_u32(&f.usecount);
  return &f;
}

static int TtyClose(File_t *f) {
  Atomic_Decrement_u32(&f->usecount);
  return 0;
}

static int TtyWrite(__unused File_t *f, const char *buf, size_t nbyte,
                    long *donep) {
  ConsoleWrite(buf, nbyte);
  *donep = nbyte;
  return 0;
}
