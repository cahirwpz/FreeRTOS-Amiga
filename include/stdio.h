#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>
#include <stddef.h>

typedef void (*putchar_t)(__reg("d0") char);

void putchar(__reg("d0") char);
void printf(const char *fmt, ...);
void kvprintf(putchar_t, const char *fmt, va_list ap);

void hexdump(void *ptr, size_t length);
#define hexdump_s(ptr) hexdump(ptr, sizeof(*ptr))

#endif /* !_STDIO_H_ */
