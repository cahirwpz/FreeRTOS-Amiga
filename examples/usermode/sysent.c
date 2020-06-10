#include <cdefs.h>
#include <cpu.h>
#include <trap.h>
#include <stdio.h>
#include "syscall.h"
#include "proc.h"

extern void vPortDefaultTrapHandler(TrapFrame_t *);

void vPortTrapHandler(TrapFrame_t *frame) {
  uint16_t sr = (CpuModel > CF_68000) ? frame->m68010.sr : frame->m68000.sr;

  /* Trap instruction from user-space ? */
  if (frame->trapnum == T_TRAPINST && (sr & SR_S) == 0) {
    Proc_t *p = GetCurrentProc();

    if (frame->d0 == SYS_exit) {
      p->exitcode = frame->d1;
      longjmp(p->retctx, 1);
    }
  }

  vPortDefaultTrapHandler(frame);
}
