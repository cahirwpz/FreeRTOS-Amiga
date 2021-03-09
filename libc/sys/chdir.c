#include <sys/syscall.h>
#include <unistd.h>

int chdir(const char *path) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  return -1;
}
