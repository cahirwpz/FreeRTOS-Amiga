#include <sys/syscall.h>
#include <unistd.h>

pid_t wait(int *statusp) {
  pid_t pid;
  SYSCALL1(pid, SYS_wait, statusp);
  return pid;
}
