#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <types.h>

#ifndef NULL
#define NULL ((void *)0)
#endif

int rand_r(unsigned *seed);

long strtol(const char *restrict str, char **restrict endptr, int base);
u_long strtoul(const char *restrict str, char **restrict endptr, int base);

#define malloc(s) pvPortMalloc(s)
#define free(p) vPortFree(p)

#define alloca(size) __builtin_alloca(size)

#endif /* !_STDLIB_H_ */
