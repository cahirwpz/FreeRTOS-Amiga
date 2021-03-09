#include <sys/syscall.h>
#include <unistd.h>

int close(int fd) {
  int err;
  SYSCALL1(err, SYS_close, fd);
  return err;
}
