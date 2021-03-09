#include <sys/syscall.h>
#include <unistd.h>

int unlink(const char *path) {
  int err;
  SYSCALL1(err, SYS_unlink, path);
  return err;
}
