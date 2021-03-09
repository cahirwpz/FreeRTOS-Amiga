#include <sys/syscall.h>
#include <unistd.h>

ssize_t write(int fd, const void *buf, size_t nbyte) {
  ssize_t res;
  SYSCALL3(res, SYS_write, fd, buf, nbyte);
  return res;
}
