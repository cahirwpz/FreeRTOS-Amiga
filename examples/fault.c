#include <FreeRTOS/FreeRTOS.h>
#include <stdio.h>

void vApplicationMallocFailedHook(void) {
  printf("Memory exhausted!\n");
  portHALT();
}

void vApplicationStackOverflowHook(void) {
  printf("Stack overflow!\n");
  portHALT();
}
