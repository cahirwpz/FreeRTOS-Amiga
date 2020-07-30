#include <stdio.h>

int snprintf(char *buf, size_t size, const char *fmt, ...) {
  char *sbuf = buf;
  char *ebuf = buf + size - 1;
  va_list ap;

  void sputchar(char c) {
    if (sbuf < ebuf)
      *sbuf++ = c;
  }

  va_start(ap, fmt);
  kvprintf(sputchar, fmt, ap);
  va_end(ap);
  if (buf != NULL)
    *sbuf = '\0';
  return sbuf - buf;
}
