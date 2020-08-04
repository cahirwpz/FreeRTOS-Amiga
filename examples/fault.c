#include <FreeRTOS/FreeRTOS.h>
#include <stdio.h>

void vApplicationMallocFailedHook(void) {
  printf("Memory exhausted!\n");
  portPANIC();
}

void vApplicationStackOverflowHook(void) {
  printf("Stack overflow!\n");
  portPANIC();
}
