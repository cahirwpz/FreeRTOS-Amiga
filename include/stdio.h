#ifndef _STDIO_H_
#define _STDIO_H_

#include <cdefs.h>
#include <stdarg.h>
#include <stddef.h>
#include <file.h>

extern File_t *KernCons; /* Kernel console file (parallel port by default) */

typedef void (*putchar_t)(char);
void kvprintf(putchar_t, const char *fmt, va_list ap);

#define putchar(c) FilePutChar(KernCons, (c))
#define printf(...) FilePrintf(KernCons, __VA_ARGS__)

void hexdump(void *ptr, size_t length);
#define hexdump_s(ptr) hexdump(ptr, sizeof(*ptr))

#endif /* !_STDIO_H_ */
