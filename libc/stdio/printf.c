#include <stdio.h>

void printf(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
  kvprintf(putchar, fmt, ap);
	va_end(ap);
}
