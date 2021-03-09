#include <sys/syscall.h>
#include <unistd.h>

ssize_t read(int fd, void *buf, size_t nbyte) {
  long res;
  SYSCALL3(res, SYS_read, fd, buf, nbyte);
  return res;
}
