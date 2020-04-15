#include <file.h>
#include "console.h"

static int ConsoleWrite(File_t *f, const char *buf, size_t nbyte);
static void ConsoleClose(File_t *f);

static FileOps_t ConsOps = {
  .write = (FileWrite_t)ConsoleWrite,
  .close = ConsoleClose
};

File_t *ConsoleOpen(void) {
  static File_t f = {.ops = &ConsOps};
  f.usecount++;
  return &f;
}

static void ConsoleClose(File_t *f) {
  f->usecount--;
}

static int ConsoleWrite(__unused File_t *f, const char *buf, size_t nbyte) {
  for (size_t i = 0; i < nbyte; i++)
    ConsolePutChar(*buf++);
  return nbyte;
}
