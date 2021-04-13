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

/* When simulator is configured to enter debugger on illegal instructions,
 * this macro can be used to set breakpoints in your code. */
#define BREAK()                                                                \
  { asm volatile("\tillegal\n"); }

/* Halt the processor by masking all interrupts and waiting for NMI. */
#define HALT()                                                                 \
  { asm volatile("\tstop\t#0x2700\n"); }

/* Use whenever a program should generate a fatal error. This will break into
 * debugger for program inspection and stop instruction execution. */
#define PANIC()                                                                \
  {                                                                            \
    BREAK();                                                                   \
    HALT();                                                                    \
  }

/* Instruction that effectively is a no-op, but its opcode is different from
 * real nop instruction. Useful for introducing transparent breakpoints that
 * are only understood by simulator. */
#define NOP()                                                                  \
  { asm volatile("\texg\t%d7,%d7\n"); }

/* Make the processor wait for interrupt. */
#define WFI()                                                                  \
  { asm volatile("\tstop\t#0x2000\n"); }

/* Issue trap 0..15 instruction that can be interpreted by a trap handler. */
#define TRAP(n)                                                                \
  { asm volatile("\ttrap\t#" #n "\n"); }

/* Read Vector Base Register (68010 and above only) */
static inline void *portGetVBR(void) {
  void *vbr;
  asm volatile("\tmovec\t%%vbr,%0\n" : "=d"(vbr));
  return vbr;
}

/* Read whole Status Register (privileged instruction on 68010 and above) */
static inline uint16_t portGetSR(void) {
  uint16_t sr;
  asm volatile("\tmove.w\t%%sr,%0\n" : "=d"(sr));
  return sr;
}
