#ifndef _CPU_H_
#define _CPU_H_

#define CB_68010 0
#define CB_68020 1
#define CB_68030 2
#define CB_68040 3
#define CB_68881 4
#define CB_68882 5
#define CB_FPU40 6
#define CB_68060 7

#define CF_68000 (0)
#define CF_68010 (1 << CB_68010)
#define CF_68020 (1 << CB_68020)
#define CF_68020 (1 << CB_68020)
#define CF_68030 (1 << CB_68030)
#define CF_68040 (1 << CB_68040)
#define CF_68881 (1 << CB_68881)
#define CF_68882 (1 << CFB_68882)
#define CF_FPU40 (1 << CFB_FPU40)
#define CF_68060 (1 << CFB_68060)

extern uint8_t CpuModel;

#endif /* !_CPU_H_ */
