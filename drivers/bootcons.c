#include <file.h>

extern void BootConsPutChar(char c);

static int BootConsWrite(__unused File_t *f, const char *buf, size_t nbyte,
                         long *donep) {
  for (size_t i = 0; i < nbyte; i++)
    BootConsPutChar(*buf++);
  *donep = nbyte;
  return 0;
}

static FileOps_t BootConsOps = {.write = (FileWrite_t)BootConsWrite};

static File_t BootCons = {.ops = &BootConsOps, .usecount = 1, .writeable = 1};

File_t *KernCons = &BootCons;
