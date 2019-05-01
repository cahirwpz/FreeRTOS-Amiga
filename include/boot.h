#ifndef _BOOT_H_
#define _BOOT_H_

#include <cdefs.h>

typedef struct MemRegion {
  uintptr_t mr_lower;
  uintptr_t mr_upper;
} MemRegion_t;  

typedef struct BootData {
  uintptr_t bd_entry;
  uintptr_t bd_vbr;
  uint16_t bd_cpumodel;
  uint16_t bd_nregions;
  MemRegion_t bd_region[];
} BootData_t;

#endif /* !_BOOT_H_ */
