#include <sys/syscall.h>
#include <unistd.h>

int chdir(const char *path) {
  int err;
  SYSCALL1(err, SYS_chdir, path);
  return err;
}
