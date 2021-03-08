#include <FreeRTOS/FreeRTOS.h>
#include <libkern.h>

void vApplicationMallocFailedHook(void) {
  kprintf("Memory exhausted!\n");
  portPANIC();
}

void vApplicationStackOverflowHook(void) {
  kprintf("Stack overflow!\n");
  portPANIC();
}
