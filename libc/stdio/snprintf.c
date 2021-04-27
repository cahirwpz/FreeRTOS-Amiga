#include <stdio.h>

typedef struct out {
  char *s, *e;
} out_t;

static void sputchar(out_t *out, char c) {
  if (out->s < out->e)
    *out->s++ = c;
}

int snprintf(char *buf, size_t size, const char *fmt, ...) {
  out_t out = {buf, buf + size - 1};
  va_list ap;

  va_start(ap, fmt);
  kvprintf((putchar_t)sputchar, &out, fmt, ap);
  va_end(ap);
  if (buf != NULL)
    *out.s = '\0';
  return out.s - buf;
}
