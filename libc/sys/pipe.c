#include <sys/syscall.h>
#include <unistd.h>

int pipe(int fd[2]) {
  int err;
  SYSCALL1(err, SYS_pipe, fd);
  return err;
}
