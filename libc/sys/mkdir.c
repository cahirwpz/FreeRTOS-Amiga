#include <sys/syscall.h>
#include <sys/stat.h>

int mkdir(const char *path) {
  int err;
  SYSCALL1(err, SYS_mkdir, path);
  return err;
}
