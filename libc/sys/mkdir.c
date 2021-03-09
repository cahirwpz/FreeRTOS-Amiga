#include <sys/syscall.h>
#include <sys/stat.h>

int mkdir(const char *path, mode_t mode) {
  int err;
  SYSCALL2(err, SYS_mkdir, path, mode);
  return err;
}
