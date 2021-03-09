#include <sys/syscall.h>
#include <unistd.h>

int unlink(const char *path) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  return -1;
}
