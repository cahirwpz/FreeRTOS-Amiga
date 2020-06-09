#include <cdefs.h>
#include <cpu.h>
#include <trap.h>
#include <stdio.h>
#include "syscall.h"
#include "proc.h"

extern void vPortDefaultTrapHandler(TrapFrame_t *);

static ProcFrame_t *GetProcFrame(TrapFrame_t *frame) {
  return (void *)frame->sp + sizeof(frame->trapnum) +
         (CpuModel > CF_68000 ? sizeof(frame->m68010) : sizeof(frame->m68000));
}

void vPortTrapHandler(TrapFrame_t *frame) {
  uint16_t sr = (CpuModel > CF_68000) ? frame->m68010.sr : frame->m68000.sr;

  /* Trap instruction from user-space ? */
  if (frame->trapnum == T_TRAPINST && (sr & SR_S) == 0) {
    ProcFrame_t *pf = GetProcFrame(frame);
    /* pf->proc stores a pointer to currently running process */

    if (frame->d0 == SYS_exit)
      ExitUserMode(pf, frame->d1);
  }

  vPortDefaultTrapHandler(frame);
}
