#include <sys/syscall.h>
#include <unistd.h>

int wait(int *statusp) {
  /* TODO: Implement system call wrapper. */
  (void)statusp;
  return -1;
}
