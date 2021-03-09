#include <sys/syscall.h>
#include <sys/stat.h>

int mkdir(const char *path, mode_t mode) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  (void)mode;
  return -1;
}
