#include <FreeRTOS.h>
#include <hardware.h>
#include <exception.h>
#include <interrupt.h>
#include <cpu.h>
#include <stdio.h>

volatile Custom_t custom = (Custom_t)0xdff000;
volatile CIA_t ciaa = (CIA_t)0xbfe001;
volatile CIA_t ciab = (CIA_t)0xbfd000;

/* Exception Vector Base: 0 for 68000, for 68010 and above read from VBR */
ExcVec_t *ExcVecBase = (ExcVec_t *)0L;
ExcVec_t *ReadVBR() = "\tmovec\tvbr,d0\n";
uint16_t GetSR() = "\tmove.w\tsr,d0\n";

/* Amiga Interrupt Vector */
IntVec_t IntVec;

extern void AmigaLvl1Handler(void);
extern void AmigaLvl2Handler(void);
extern void AmigaLvl3Handler(void);
extern void AmigaLvl4Handler(void);
extern void AmigaLvl5Handler(void);
extern void AmigaLvl6Handler(void);

/* Value of this variable is provided by the boot loader. */
uint8_t CpuModel = 0;

__entry void _start(uint8_t aCpuModel, const HeapRegion_t *const xHeapRegions) {
  dprintf("FreeRTOS running on Amiga!\n");

  configASSERT(custom->intenar == 0);
  configASSERT((GetSR() & 0x2700) == 0x2700);

  CpuModel = aCpuModel;

  if (CpuModel & CF_68010)
    ExcVecBase = ReadVBR();

  /* Initialize M68k interrupt vector. */
  for (int i = EV_BUSERR; i <= EV_LAST; i++)
    ExcVec[i] = vDummyExceptionHandler;

  /* Initialize level 1-7 interrupt autovector in Amiga specific way. */
  ExcVec[EV_INTLVL(1)] = AmigaLvl1Handler;
  ExcVec[EV_INTLVL(2)] = AmigaLvl2Handler;
  ExcVec[EV_INTLVL(3)] = AmigaLvl3Handler;
  ExcVec[EV_INTLVL(4)] = AmigaLvl4Handler;
  ExcVec[EV_INTLVL(5)] = AmigaLvl5Handler;
  ExcVec[EV_INTLVL(6)] = AmigaLvl6Handler;

  vPortDefineHeapRegions(xHeapRegions);

  extern int main(void);
  main();
}
