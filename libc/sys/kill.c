#include <sys/syscall.h>
#include <signal.h>

int kill(pid_t pid) {
  /* TODO: Implement system call wrapper. */
  (void)pid;
  return -1;
}
