#include <FreeRTOS/FreeRTOS.h>
#include <stdio.h>

void vApplicationMallocFailedHook(void) {
  printf("Memory exhausted!\n");
  portBREAK();
  portHALT();
}

void vApplicationStackOverflowHook(void) {
  printf("Stack overflow!\n");
  portBREAK();
  portHALT();
}
