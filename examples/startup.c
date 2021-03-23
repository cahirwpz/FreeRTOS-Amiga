#include <FreeRTOS/FreeRTOS.h>
#include <custom.h>
#include <cpu.h>
#include <cia.h>
#include <boot.h>
#include <libkern.h>

#define DEBUG 0
#include <debug.h>

extern int main(void);

void _start(BootData_t *aBootData) {
  kprintf("FreeRTOS running on Amiga!\n");
  kprintf("VBR at $%08x\n", aBootData->bd_vbr);
  kprintf("CPU model $%02x\n", aBootData->bd_cpumodel);
  kprintf("Entry point at $%08x\n", aBootData->bd_entry);

  for (int i = 0; i < aBootData->bd_nregions; i++)
    kprintf("MEM[%d]: %08x - %08x\n", i, aBootData->bd_region[i].mr_lower,
            aBootData->bd_region[i].mr_upper);

  DASSERT(custom.intenar == 0);
  DASSERT((custom.dmaconr & DMAF_ALL) == 0);
  DASSERT((portGetSR() & 0x2700) == 0x2700);

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
