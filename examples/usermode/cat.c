#include "sysapi.h"

static int cat(int fd) {
  static char buf[1024];

  for (;;) {
    int n = read(fd, buf, sizeof(buf));
    if (n < 0)
      return 1;
    if (n == 0)
      break;
    if (write(STDOUT_FILENO, buf, n) != n)
      return 1;
  }

  return 0;
}

int main(int argc, char *argv[]) {
  if (argc == 1)
    return cat(0);

  for (int i = 1; i < argc; i++) {
    int fd = open(argv[i]);
    if (fd < 0)
      return 1;
    if (cat(fd))
      return 1;
    close(fd);
  }

  return 0;
}
