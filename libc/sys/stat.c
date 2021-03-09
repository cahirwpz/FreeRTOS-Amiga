#include <sys/syscall.h>
#include <sys/stat.h>

int stat(const char *path, struct stat *sb) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  (void)sb;
  return -1;
}
