#pragma once

#include <sys/types.h>
#include <stddef.h>

#define alloca(size) __builtin_alloca(size)

int atoi(const char *);
int rand_r(unsigned int *seed);
long strtol(const char *restrict str, char **restrict endptr, int base);
u_long strtoul(const char *restrict str, char **restrict endptr, int base);

#ifdef _USERSPACE

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

__noreturn void exit(int);
void free(void *);
void *malloc(size_t);
void *realloc(void *ptr, size_t size);

#endif /* !_USERSPACE */
