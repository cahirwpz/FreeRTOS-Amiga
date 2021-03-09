#include <sys/syscall.h>
#include <stdlib.h>

void exit(int status) {
  SYSCALL1_NR(SYS_exit, status);
  for (;;)
    continue;
}
