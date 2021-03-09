#pragma once

#include <sys/cdefs.h>
#include <stdarg.h>
#include <stddef.h>

#ifndef SEEK_SET
#define SEEK_SET 0 /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR 1 /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END 2 /* set file offset to EOF plus offset */
#endif

typedef void (*putchar_t)(char);
void kvprintf(putchar_t, const char *fmt, va_list ap);
int snprintf(char *str, size_t size, const char *format, ...);

#ifdef _USERSPACE

int dprintf(int fd, const char *fmt, ...);

#endif /* !_USERSPACE */
