#pragma once

#include <sys/cdefs.h>

#define CB_68010 0
#define CB_68020 1
#define CB_68030 2
#define CB_68040 3
#define CB_68881 4
#define CB_68882 5
#define CB_FPU40 6
#define CB_68060 7

/* Describe available processor features.
 * Refer to user manual for given model. */
typedef enum CpuModel {
  CF_68000 = 0,
  CF_68010 = BIT(CB_68010),
  CF_68020 = BIT(CB_68020),
  CF_68030 = BIT(CB_68030), /* memory management unit available */
  CF_68040 = BIT(CB_68040),
  CF_68881 = BIT(CB_68881), /* floating point unit available */
  CF_68882 = BIT(CB_68882),
  CF_FPU40 = BIT(CB_FPU40),
  CF_68060 = BIT(CB_68060),
} __packed CpuModel_t;

/* Used when there are differences between processors
 * and there's need to handle certain features separately. */
extern CpuModel_t CpuModel;
