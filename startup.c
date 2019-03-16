#include <FreeRTOS.h>
#include <hardware.h>
#include <evec.h>
#include <cpu.h>
#include <libsa.h>

volatile struct Custom* const custom = (APTR)0xdff000;

/* Exception Vector Base: 0 for 68000, for 68010 and above read from VBR */
ExcVec_t *ExcVecBase = (ExcVec_t *)0L;
ExcVec_t *ReadVBR() = "\tmovec\tvbr,d0\n";
uint16_t GetSR() = "\tmove.w\tsr,d0\n";

/* Value of this variable is provided by the boot loader. */
uint8_t CpuModel = 0;

__entry void _start(uint8_t aCpuModel, const HeapRegion_t *const xHeapRegions) {
  dprintf("FreeRTOS running on Amiga!\n");

  configASSERT(custom->intenar == 0);
  configASSERT((GetSR() & 0x2700) == 0x2700);

  CpuModel = aCpuModel;

  if (CpuModel & CF_68010)
    ExcVecBase = ReadVBR();

  /* Initialize interrupt vector. */
  for (int i = EV_BUSERR; i <= EV_LAST; i++)
    ExcVec[i] = vDummyExceptionHandler;

  vPortDefineHeapRegions(xHeapRegions);

  extern int main(void);
  main();
}
