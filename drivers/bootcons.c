#include <file.h>

extern void BootConsPutChar(char c);

static long BootConsWrite(__unused File_t *f, const char *buf, size_t nbyte) {
  for (size_t i = 0; i < nbyte; i++)
    BootConsPutChar(*buf++);
  return nbyte;
}

static FileOps_t BootConsOps = {
  .write = (FileWrite_t)BootConsWrite
};

static File_t BootCons = {.ops = &BootConsOps, .usecount = 1};

File_t *KernCons = &BootCons;
