#include <FreeRTOS/FreeRTOS.h>
#include <custom.h>
#include <cpu.h>
#include <cia.h>
#include <boot.h>
#include <stdio.h>

extern int main(void);

void _start(BootData_t *aBootData) {
  printf("FreeRTOS running on Amiga!\n");
  printf("VBR at $%08x\n", aBootData->bd_vbr);
  printf("CPU model $%02x\n", aBootData->bd_cpumodel);
  printf("Entry point at $%08x\n", aBootData->bd_entry);

  for (int i = 0; i < aBootData->bd_nregions; i++)
    printf("MEM[%d]: %08x - %08x\n", i,
           aBootData->bd_region[i].mr_lower, aBootData->bd_region[i].mr_upper);

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
