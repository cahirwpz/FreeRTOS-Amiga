#ifndef _STRINGS_H_
#define _STRINGS_H_

#include <stddef.h>

void bzero(void *s, size_t n);
int ffs(int value);
char *index(const char *s, int c);
char *rindex(const char *s, int c);

#endif /* !_STRINGS_H_ */
