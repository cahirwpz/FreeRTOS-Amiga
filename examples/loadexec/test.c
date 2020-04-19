#include <custom.h>

int a[10];

int *ap = a + 5;

void _start(void) {
  for(;;) {
    custom.color[0] = *ap;
  }
}
