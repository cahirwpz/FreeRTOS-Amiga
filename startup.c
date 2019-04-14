#include <FreeRTOS/FreeRTOS.h>
#include <custom.h>
#include <cpu.h>
#include <boot.h>
#include <stdio.h>

extern int main(void);

void _start(BootData_t *aBootData) {
  printf("FreeRTOS running on Amiga!\n");

  configASSERT(custom->intenar == 0);
  configASSERT((custom->dmaconr & DMAF_ALL) == 0);
  configASSERT((portGetSR() & 0x2700) == 0x2700);

  CpuModel = aBootData->bd_cpumodel;

  vPortSetupHardware();
  vPortSetupExceptionVector();
  vPortDefineMemoryRegions(aBootData->bd_region);

  main();
}
