#include <sys/syscall.h>
#include <sys/ioctl.h>

int _ioctl(int fd, u_long cmd, void *data) {
  int res;
  SYSCALL3(res, SYS_ioctl, fd, cmd, data);
  return res;
}

__strong_alias(ioctl, _ioctl);
