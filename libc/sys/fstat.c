#include <sys/syscall.h>
#include <sys/stat.h>

int fstat(int fd, struct stat *sb) {
  int err;
  SYSCALL2(err, SYS_fstat, fd, sb);
  return err;
}
