#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char buf[512];

static void wc(int fd, char *name) {
  int i, n;
  int l, w, c, inword;

  l = w = c = 0;
  inword = 0;
  while ((n = read(fd, buf, sizeof(buf))) > 0) {
    for (i = 0; i < n; i++) {
      c++;
      if (buf[i] == '\n')
        l++;
      if (strchr(" \r\t\n\v", buf[i]))
        inword = 0;
      else if (!inword) {
        w++;
        inword = 1;
      }
    }
  }
  if (n < 0) {
    dprintf(STDERR_FILENO, "wc: read error\n");
    exit(1);
  }
  dprintf(STDOUT_FILENO, "%d %d %d %s\n", l, w, c, name);
}

int main(int argc, char *argv[]) {
  int fd, i;

  if (argc <= 1) {
    wc(0, "");
    exit(0);
  }

  for (i = 1; i < argc; i++) {
    if ((fd = open(argv[i], 0)) < 0) {
      dprintf(STDERR_FILENO, "wc: cannot open %s\n", argv[i]);
      exit(1);
    }
    wc(fd, argv[i]);
    close(fd);
  }
  exit(0);
}
