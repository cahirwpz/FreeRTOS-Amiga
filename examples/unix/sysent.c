#include <cpu.h>
#include <trap.h>
#include <string.h>

#include <sys/syscall.h>
#include <sys/proc.h>

extern void vPortDefaultTrapHandler(TrapFrame_t *);

void CloneUserCtx(UserCtx_t *ctx, TrapFrame_t *frame) {
  ctx->pc = (CpuModel > CF_68000) ? frame->m68010.pc : frame->m68000.pc;
  ctx->sp = frame->usp;
  memcpy(&ctx->d0, &frame->d0, 15 * sizeof(uint32_t));
}

void vPortTrapHandler(TrapFrame_t *frame) {
  uint16_t sr = (CpuModel > CF_68000) ? frame->m68010.sr : frame->m68000.sr;

  /* Trap instruction from user-space ? */
  if (frame->trapnum == T_TRAPINST && (sr & SR_S) == 0) {
    Proc_t *p = TaskGetProc();

    /* Handle known system calls! */
    if (frame->d0 == SYS_exit)
      ProcExit(p, frame->d1);
  }

  /* Everything else goes to default trap handler. */
  vPortDefaultTrapHandler(frame);
}
