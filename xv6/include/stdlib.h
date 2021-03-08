#pragma once

#include <sys/types.h>

#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

int atoi(const char *);
__noreturn void exit(int);
void free(void *);
void *malloc(size_t);
