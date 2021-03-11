#include <pipe.h>
#include <sys/errno.h>

int PipeAlloc(File_t **rfilep, File_t **wfilep) {
  (void)rfilep, (void)wfilep;
  return ENOSYS;
}
