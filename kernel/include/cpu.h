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

#define CF_68000 (0)
#define CF_68010 BIT(CB_68010)
#define CF_68020 BIT(CB_68020)
#define CF_68020 BIT(CB_68020)
#define CF_68030 BIT(CB_68030)
#define CF_68040 BIT(CB_68040)
#define CF_68881 BIT(CB_68881)
#define CF_68882 BIT(CFB_68882)
#define CF_FPU40 BIT(CFB_FPU40)
#define CF_68060 BIT(CFB_68060)

extern uint8_t CpuModel;
