#include <cdefs.h>
#include <cpu.h>
#include <trap.h>
#include <stdio.h>
#include "syscall.h"
#include "usermode.h"

extern void vPortDefaultTrapHandler(struct TrapFrame *);

__noreturn void SysExit(struct TrapFrame *frame) {
  uintptr_t ctx =
    frame->sp + sizeof(frame->trapnum) +
    (CpuModel > CF_68000 ? sizeof(frame->m68010) : sizeof(frame->m68000));

  ExitUserMode(ctx, frame->d1);
}

void vPortTrapHandler(struct TrapFrame *frame) {
  if (frame->trapnum == T_TRAPINST) {
    if (frame->d0 == SYS_exit)
      SysExit(frame);
  }

  vPortDefaultTrapHandler(frame);
}
