#pragma once

#include <sys/cdefs.h>

/* Describes memory region to be managed by OS memory allocator. */
typedef struct MemRegion {
  uintptr_t mr_lower;
  uintptr_t mr_upper;
} MemRegion_t;

/* Structure prepared by the boot loader and pushed on stack
 * before passing control to Scheduler Task. */
typedef struct BootData {
  uintptr_t bd_entry;   /* first instruction to execute */
  uintptr_t bd_vbr;     /* where exception vector begins */
  uint16_t bd_cpumodel; /* model number one of CF_* from <cpu.h> */
  uint16_t bd_nregions; /* number of memory regions to be managed */
  MemRegion_t bd_region[];
} BootData_t;
