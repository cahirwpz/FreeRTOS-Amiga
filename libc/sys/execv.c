#include <sys/syscall.h>
#include <unistd.h>

int execv(const char *path, char *const argv[]) {
  int err;
  SYSCALL2(err, SYS_execv, path, argv);
  return err;
}
