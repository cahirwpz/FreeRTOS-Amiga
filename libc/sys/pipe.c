#include <sys/syscall.h>
#include <unistd.h>

int pipe(int fd[2]) {
  /* TODO: Implement system call wrapper. */
  (void)fd;
  return -1;
}
