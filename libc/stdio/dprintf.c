#include <stdio.h>
#include <unistd.h>

int dprintf(int fd, const char *fmt, ...) {
  size_t n = 0;
  va_list ap;

  void dputchar(char c) {
    write(fd, &c, 1);
    n++;
  }

  va_start(ap, fmt);
  kvprintf(dputchar, fmt, ap);
  va_end(ap);
  return n;
}
