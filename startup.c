#include <FreeRTOS.h>
#include <hardware.h>
#include <evec.h>
#include <cpu.h>
#include <libsa.h>

volatile struct Custom* const custom = (APTR)0xdff000;

/* Exception Vector Base: 0 for 68000, for 68010 and above read from VBR */
ExcVec_t *ExcVecBase = (ExcVec_t *)0L;
ExcVec_t *ReadVBR() = "\tmovec\tvbr,d0\n";

/* Value of this variable is provided by the boot loader. */
uint8_t CpuModel = 0;

__entry void _start(uint8_t aCpuModel, const HeapRegion_t *const xHeapRegions) {
  dprintf("FreeRTOS running on Amiga!\n");

  CpuModel = aCpuModel;

  if (CpuModel & CF_68010)
    ExcVecBase = ReadVBR();

  vPortDefineHeapRegions(xHeapRegions);

  extern int main(void);
  main();
}
