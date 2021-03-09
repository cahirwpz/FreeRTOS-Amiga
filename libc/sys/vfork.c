#include <sys/syscall.h>
#include <unistd.h>

pid_t vfork(void) {
  pid_t pid;
  SYSCALL0(pid, SYS_vfork);
  return pid;
}
