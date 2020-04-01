#include <stdint.h>
#include <stdio.h>

void hexdump(void *ptr, size_t length) {
  unsigned char *data = ptr;
  for (size_t i = 0; i < length; i++) {
    if ((i & 15) == 0)
      printf("%08x:", (intptr_t)data);
    unsigned char byte = *data++;
    printf(" %02x", (int)byte);
    if ((i & 3) == 3)
      putchar(' ');
    if ((i & 15) == 15)
      putchar('\n');
  }
  putchar('\n');
}
