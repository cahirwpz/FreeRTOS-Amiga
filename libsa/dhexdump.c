#include <stdint.h>
#include <stdio.h>

void dhexdump(void *ptr, size_t length) {
  unsigned char *data = ptr;
  for (int i = 0; i < length; i++) {
    if ((i & 15) == 0)
      dprintf("%08lx:", (intptr_t)data);
    unsigned char byte = *data++;
    dprintf(" %02lx", (int)byte);
    if ((i & 3) == 3)
      dputchar(' ');
    if ((i & 15) == 15)
      dputchar('\n');
  }
  dputchar('\n');
}
