#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <amigahunk.h>
#include <cpu.h>
#include <trap.h>
#include <memory.h>
#include <string.h>
#include <strings.h>
#include <proc.h>

Proc_t *TaskGetProc(void) {
  return pvTaskGetThreadLocalStoragePointer(NULL, TLS_PROC);
}

void TaskSetProc(Proc_t *proc) {
  vTaskSetThreadLocalStoragePointer(NULL, TLS_PROC, (void *)proc);
}

void CloneUserCtx(UserCtx_t *ctx, TrapFrame_t *frame) {
  ctx->pc = (CpuModel > CF_68000) ? frame->m68010.pc : frame->m68000.pc;
  ctx->sp = frame->usp;
  memcpy(&ctx->d0, &frame->d0, 15 * sizeof(uint32_t));
}

int ProcLoadImage(Proc_t *proc, File_t *exe) {
  Hunk_t *hunk = LoadHunkList(exe);
  FileClose(exe);

  if (hunk == NULL)
    return 0;
  proc->hunk = hunk;

  /* We assume that _start procedure is placed
   * at the beginning of first hunk of executable file. */
  proc->usrctx.pc = (intptr_t)proc->hunk->data;
  return 1;
}

void ProcFreeImage(Proc_t *proc) {
  Hunk_t *next;
  for (Hunk_t *hunk = proc->hunk; hunk != NULL; hunk = next) {
    next = hunk->next;
    MemFree(hunk);
  }
}

void ProcInit(Proc_t *proc, size_t ustksz) {
  static int pid = 1; /* let's assume it will never overflow */

  bzero(proc, sizeof(Proc_t));

  /* Align to long word size. */
  ustksz = (ustksz + 3) & -4;
  proc->ustksz = ustksz;
  proc->ustk = MemAlloc(ustksz, 0);
  bzero(proc->ustk, ustksz);

  proc->pid = pid++;
}

void ProcFini(Proc_t *proc) {
  ProcFreeImage(proc);

  for (int i = 0; i < MAXFILES; i++) {
    File_t *f = proc->fdtab[i];
    if (f != NULL)
      FileClose(f);
  }

  MemFree(proc->ustk);
}

#define PUSH(sp, v)                                                            \
  {                                                                            \
    uint32_t *__sp = sp;                                                       \
    *(--__sp) = (uint32_t)v;                                                   \
    sp = __sp;                                                                 \
  }

/* Copy argv contents to user stack and create argc and argv. */
void ProcSetArgv(Proc_t *proc, char *const *argv) {
  void *sp = proc->ustk + proc->ustksz; /* Stack grows down. */
  char **uargv;
  char *uargs;
  int argc;

  for (argc = 0, uargs = sp; argv[argc]; argc++)
    uargs -= strlen(argv[argc]) + 1;

  /* align stack pointer to long word boundary */
  uargs = (void *)((intptr_t)uargs & ~3);
  /* make space for NULL terminated pointer array */
  uargv = (char **)uargs - (argc + 1);

  for (int i = 0; i < argc; i++) {
    int sz = strlen(argv[i]) + 1;
    uargv[i] = memcpy(uargs, argv[i], sz);
    uargs += sz;
  }

  uargv[argc] = NULL; /* argv terminator */

  /* push pointer to uargv and argc on stack */
  sp = uargv;
  PUSH(sp, uargv);
  PUSH(sp, argc);

  proc->usrctx.sp = (intptr_t)sp;
}

void ProcEnter(Proc_t *proc) {
  if (!setjmp(proc->retctx))
    EnterUserMode(&proc->usrctx);
}

__noreturn void ProcExit(Proc_t *proc, int exitcode) {
  proc->exitcode = exitcode;
  longjmp(proc->retctx, 1);
}
