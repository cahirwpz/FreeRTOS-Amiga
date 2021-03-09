// init: The initial user-level program

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char *argv[] = {"sh", 0};

int main(void) {
  int pid, wpid;

  open("console", O_RDWR);
  dup(0); // stdout
  dup(0); // stderr

  for (;;) {
    dprintf(STDOUT_FILENO, "init: starting sh\n");
    pid = vfork();
    if (pid < 0) {
      dprintf(STDERR_FILENO, "init: fork failed\n");
      exit(1);
    }
    if (pid == 0) {
      execv("sh", argv);
      dprintf(STDERR_FILENO, "init: exec sh failed\n");
      exit(1);
    }

    for (;;) {
      // this call to wait() returns if the shell exits,
      // or if a parentless process exits.
      wpid = wait((int *)0);
      if (wpid == pid) {
        // the shell exited; restart it.
        break;
      } else if (wpid < 0) {
        dprintf(STDERR_FILENO, "init: wait returned an error\n");
        exit(1);
      } else {
        // it was a parentless process; do nothing.
      }
    }
  }
}
