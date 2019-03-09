#include <FreeRTOS.h>
#include <hardware.h>

volatile struct Custom* const custom = (APTR)0xdff000;

extern int main(void);

__entry void _start(void) {
  main();
}
