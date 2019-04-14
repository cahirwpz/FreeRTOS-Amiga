#ifndef _STDIO_H_
#define _STDIO_H_

#include <cdefs.h>
#include <stdarg.h>
#include <stddef.h>

typedef void (*putchar_t)(char);

void putchar(char);
void printf(const char *fmt, ...);
void kvprintf(putchar_t, const char *fmt, va_list ap);

void hexdump(void *ptr, size_t length);
#define hexdump_s(ptr) hexdump(ptr, sizeof(*ptr))

#endif /* !_STDIO_H_ */
