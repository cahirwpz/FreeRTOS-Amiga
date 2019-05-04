#include <FreeRTOS/FreeRTOS.h>
#include <custom.h>
#include <cpu.h>
#include <cia.h>
#include <boot.h>
#include <stdio.h>

extern int main(void);

void _start(BootData_t *aBootData) {
  printf("FreeRTOS running on Amiga!\n");

  configASSERT(custom.intenar == 0);
  configASSERT((custom.dmaconr & DMAF_ALL) == 0);
  configASSERT((portGetSR() & 0x2700) == 0x2700);

  CpuModel = aBootData->bd_cpumodel;

  vPortSetupExceptionVector(aBootData);
  vPortDefineMemoryRegions(aBootData->bd_region);

  /* CIA-A & CIA-B: Stop timers and return to default settings. */
  ciaa.ciacra = 0;
  ciaa.ciacrb = 0;
  ciab.ciacra = 0;
  ciab.ciacrb = 0;

  /* CIA-A & CIA-B: Clear pending interrupts. */
  SampleICR(CIAA, CIAICRF_ALL);
  SampleICR(CIAB, CIAICRF_ALL);

  /* CIA-A & CIA-B: Disable all interrupts. */
  WriteICR(CIAA, CIAICRF_ALL);
  WriteICR(CIAB, CIAICRF_ALL);

  /* Enable master bit in DMACON */
  EnableDMA(DMAF_MASTER);

  main();
}
