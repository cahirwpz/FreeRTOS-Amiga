#include "tty.h"
#include "console.h"

static int TtyWrite(File_t *f, const char *buf, size_t nbyte);
static void TtyClose(File_t *f);

static FileOps_t TtyOps = {
  .write = (FileWrite_t)TtyWrite,
  .close = TtyClose
};

File_t *TtyOpen(void) {
  static File_t f = {.ops = &TtyOps};
  f.usecount++;
  return &f;
}

static void TtyClose(File_t *f) {
  f->usecount--;
}

static int TtyWrite(__unused File_t *f, const char *buf, size_t nbyte) {
  for (size_t i = 0; i < nbyte; i++)
    ConsolePutChar(*buf++);
  return nbyte;
}
