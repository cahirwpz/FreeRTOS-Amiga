#include <FreeRTOS.h>
#include <hardware.h>
#include <evec.h>
#include <libsa.h>

volatile struct Custom* const custom = (APTR)0xdff000;

ExcVec_t *ExcVecBase = (ExcVec_t *)0L;

extern int main(void);

__entry void _start(const HeapRegion_t *const xHeapRegions) {
  dprintf("FreeRTOS running on Amiga!\n");

  for (int i = 0; xHeapRegions[i].pucStartAddress; i++)
    dprintf("HeapRegion[%d]: ($%08x,%d)\n", i,
            (intptr_t)xHeapRegions[i].pucStartAddress,
            xHeapRegions[i].xSizeInBytes);
 
  vPortDefineHeapRegions(xHeapRegions);

  dprintf("FreeHeapSize: %d\n", xPortGetFreeHeapSize());

  main();
}
