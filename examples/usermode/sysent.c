#include <cpu.h>
#include <trap.h>

#include "syscall.h"
#include "proc.h"

extern void vPortDefaultTrapHandler(TrapFrame_t *);

void vPortTrapHandler(TrapFrame_t *frame) {
  uint16_t sr = (CpuModel > CF_68000) ? frame->m68010.sr : frame->m68000.sr;

  /* Trap instruction from user-space ? */
  if (frame->trapnum == T_TRAPINST && (sr & SR_S) == 0) {
    Proc_t *p = TaskGetProc();

    /* Handle known system calls! */
    if (frame->d0 == SYS_exit)
      ExitUserMode(p, frame->d1);
  }

  /* Everything else goes to default trap handler. */
  vPortDefaultTrapHandler(frame);
}
