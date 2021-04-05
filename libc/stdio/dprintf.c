#include <stdio.h>
#include <unistd.h>

typedef struct out {
  size_t n;
  int fd;
} out_t;

static void dputchar(out_t *out, char c) {
  write(out->fd, &c, 1);
  out->n++;
}

int dprintf(int fd, const char *fmt, ...) {
  out_t out = {0, fd};
  va_list ap;

  va_start(ap, fmt);
  kvprintf((putchar_t)dputchar, &out, fmt, ap);
  va_end(ap);
  return out.n;
}
