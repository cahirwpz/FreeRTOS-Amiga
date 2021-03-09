#include <sys/syscall.h>
#include <unistd.h>

int dup(int fd) {
  int fd2;
  SYSCALL1(fd2, SYS_dup, fd);
  return fd2;
}
