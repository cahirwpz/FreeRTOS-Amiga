#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <sys/types.h>
#include <stddef.h>

int rand_r(unsigned int *seed);

long strtol(const char *restrict str, char **restrict endptr, int base);
u_long strtoul(const char *restrict str, char **restrict endptr, int base);

#define alloca(size) __builtin_alloca(size)

#endif /* !_STDLIB_H_ */
