#include <FreeRTOS.h>

extern int main(void);

void _start(void) {
  main();
}
