#include <sys/syscall.h>
#include <signal.h>

int kill(pid_t pid) {
  int err;
  SYSCALL1(err, SYS_kill, pid);
  return err;
}
