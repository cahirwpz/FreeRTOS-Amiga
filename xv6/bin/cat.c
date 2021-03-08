#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char buf[512];

static void cat(int fd) {
  int n;

  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    if (write(STDOUT_FILENO, buf, n) != n) {
      dprintf(STDERR_FILENO, "cat: write error\n");
      exit(1);
    }
  }
  if (n < 0) {
    dprintf(STDERR_FILENO, "cat: read error\n");
    exit(1);
  }
}

int main(int argc, char *argv[]) {
  int fd, i;

  if (argc <= 1) {
    cat(STDIN_FILENO);
    exit(0);
  }

  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], 0)) < 0) {
      dprintf(STDERR_FILENO, "cat: cannot open %s\n", argv[i]);
      exit(EXIT_FAILURE);
    }
    cat(fd);
    close(fd);
  }

  exit(0);
}
