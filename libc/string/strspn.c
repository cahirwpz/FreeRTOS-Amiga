/*
 * This code is covered by BSD license.
 * It was taken from OpenBSD lib/libc/string/strspn.c
 */

#include <string.h>

size_t strspn(const char *s1, const char *s2) {
  const char *p = s1, *spanp;
  char c, sc;

  /* Skip any characters in s2, excluding the terminating \0. */
cont:
  c = *p++;
  for (spanp = s2; (sc = *spanp++) != 0;)
    if (sc == c)
      goto cont;
  return (p - 1 - s1);
}
