#ifndef _LIBSA_H_
#define _LIBSA_H_

#include <stdint.h>
#include <stdarg.h>

typedef void (*putchar_t)(__reg("d0") int);

void dprintf(const char *fmt, ...);
void kvprintf(putchar_t, const char *fmt, va_list ap);

#endif
