#include <FreeRTOS.h>
#include <hardware.h>
#include <libsa.h>

volatile struct Custom* const custom = (APTR)0xdff000;

extern int main(void);

__entry void _start(void) {
  dprintf("FreeRTOS running on Amig!\n");
  main();
}
