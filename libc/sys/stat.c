#include <sys/syscall.h>
#include <sys/stat.h>

int stat(const char *path, struct stat *sb) {
  int err;
  SYSCALL2(err, SYS_stat, path, sb);
  return err;
}
