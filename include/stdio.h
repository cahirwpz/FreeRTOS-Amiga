#ifndef _STDIO_H_
#define _STDIO_H_

#include <stdarg.h>
#include <stddef.h>

typedef void (*putchar_t)(__reg("d0") char);

#pragma printflike dprintf
void dputchar(__reg("d0") char);
void dprintf(const char *fmt, ...);
void kvprintf(putchar_t, const char *fmt, va_list ap);

void dhexdump(void *ptr, size_t length);
#define dhexdump_s(ptr) dhexdump(ptr, sizeof(*ptr))

#endif /* !_STDIO_H_ */
