#include <sys/syscall.h>
#include <unistd.h>

int execv(const char *path, char *const argv[]) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  (void)argv;
  return -1;
}
