#ifndef _STDLIB_H_
#define _STDLIB_H_

#include <types.h>
#include <stddef.h>

int rand_r(unsigned *seed);

long strtol(const char *restrict str, char **restrict endptr, int base);
u_long strtoul(const char *restrict str, char **restrict endptr, int base);

void *malloc(size_t size);
void free(void *ptr);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void mcheck(int verbose);

#define alloca(size) __builtin_alloca(size)

__strong_alias(malloc, pvPortMalloc);
__strong_alias(free, vPortFree);
__strong_alias(realloc, pvPortRealloc);
__strong_alias(mcheck, vPortMemCheck);

#endif /* !_STDLIB_H_ */
