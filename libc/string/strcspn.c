/*
 * This code is covered by BSD license.
 * It was taken from OpenBSD lib/libc/string/strcspn.c
 */

#include <string.h>

size_t strcspn(const char *s1, const char *s2) {
  const char *p, *spanp;
  char c, sc;

  /*
   * Stop as soon as we find any character from s2.  Note that there
   * must be a NUL in s2; it suffices to stop when we find that, too.
   */
  for (p = s1;;) {
    c = *p++;
    spanp = s2;
    do {
      if ((sc = *spanp++) == c)
        return (p - 1 - s1);
    } while (sc != 0);
  }
}
