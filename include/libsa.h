#ifndef _LIBSA_H_
#define _LIBSA_H_

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

typedef void (*putchar_t)(__reg("d0") int);

#pragma printflike dprintf
void dputchar(__reg("d0") int);
void dprintf(const char *fmt, ...);
void kvprintf(putchar_t, const char *fmt, va_list ap);

void dhexdump(void *ptr, size_t length);
#define dhexdump_s(ptr) dhexdump(ptr, sizeof(*ptr))

void bzero(void *s, size_t n);
void *memcpy(void *restrict dst, const void *restrict src, size_t n);
void *memmove(void *dst, const void *src, size_t len);
size_t strlen(const char *s);

#define malloc(s) pvPortMalloc(s)
#define free(p) pvPortFree(p)

#endif /* !_LIBSA_H_ */
