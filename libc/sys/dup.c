#include <sys/syscall.h>
#include <unistd.h>

int dup(int fd) {
  /* TODO: Implement system call wrapper. */
  (void)fd;
  return -1;
}
