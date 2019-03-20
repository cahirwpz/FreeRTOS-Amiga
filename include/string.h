#ifndef _STRING_H_
#define _STRING_H_

#include <stddef.h>

void *memcpy(void *restrict dst, const void *restrict src, size_t n);
void *memmove(void *dst, const void *src, size_t len);
void *memset(void *dst, int c, size_t len);
size_t strlen(const char *s);

#endif /* !_STRING_H_ */
