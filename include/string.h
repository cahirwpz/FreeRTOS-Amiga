#ifndef _STRING_H_
#define _STRING_H_

#include <stddef.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t n);
void *memmove(void *dst, const void *src, size_t len);
void *memset(void *dst, int c, size_t len);
char *strcpy(char *dst, const char *src);
char *strcat(char *restrict s1, const char *restrict s2);
int strcmp(const char *s1, const char *s2);
char *strchr(const char *s, int c);
int strncmp(const char *s1, const char *s2, size_t n);
char *strncpy(char *dst, const char *src, size_t len);
char *strrchr(const char *s, int c);
size_t strlen(const char *s);

size_t strcspn(const char *s1, const char *s2);
size_t strspn(const char *s1, const char *s2);
char *strtok_r(char *s, const char *delim, char **last);

#endif /* !_STRING_H_ */
