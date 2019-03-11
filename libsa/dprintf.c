#include "libsa.h"

extern void DPutChar(int);

void dprintf(const char *fmt, ...) {
	va_list ap;

	va_start(ap, fmt);
  kvprintf(DPutChar, fmt, ap);
	va_end(ap);
}
