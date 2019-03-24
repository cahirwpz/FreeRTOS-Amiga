#include <FreeRTOS.h>
#include <custom.h>
#include <cpu.h>
#include <stdio.h>

__entry void _start(uint8_t aCpuModel, const HeapRegion_t *const xHeapRegions) {
  printf("FreeRTOS running on Amiga!\n");

  configASSERT(custom->intenar == 0);
  configASSERT((portGetSR() & 0x2700) == 0x2700);

  CpuModel = aCpuModel;

  vPortSetupExceptionVector();
  vPortDefineHeapRegions(xHeapRegions);

  extern int main(void);
  main();
}
