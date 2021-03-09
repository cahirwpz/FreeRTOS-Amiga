#include <sys/syscall.h>
#include <sys/fcntl.h>

int open(const char *path, int mode) {
  int fd;
  SYSCALL2(fd, SYS_open, path, mode);
  return fd;
}
