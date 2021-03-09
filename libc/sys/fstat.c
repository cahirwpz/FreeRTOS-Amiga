#include <sys/syscall.h>
#include <sys/stat.h>

int fstat(int fd, struct stat *sb) {
  /* TODO: Implement system call wrapper. */
  (void)fd;
  (void)sb;
  return -1;
}
