#include "libsa.h"

void dprintf(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
  kvprintf(dputchar, fmt, ap);
	va_end(ap);
}
