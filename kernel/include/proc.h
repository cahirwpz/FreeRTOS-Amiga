#pragma once

#include <sys/cdefs.h>
#include <stddef.h>
#include <setjmp.h>

#define TLS_PROC 0
#define MAXFILES 16

#define UPROC_STKSZ (configMINIMAL_STACK_SIZE * 8)
#define KPROC_STKSZ (configMINIMAL_STACK_SIZE * 2)

typedef struct File File_t;
typedef struct Hunk Hunk_t;
typedef struct TrapFrame TrapFrame_t;

/* User mode context handling stuff. */
typedef struct UserCtx {
  /* SR is always set to zero, no need to save it.
   * EnterUserMode and CloneUserCtx depend on layout of this structure! */
  uint32_t pc, sp;
  uint32_t d0, d1, d2, d3, d4, d5, d6, d7;
  uint32_t a0, a1, a2, a3, a4, a5, a6;
} UserCtx_t;

void CloneUserCtx(UserCtx_t *ctx, TrapFrame_t *frame);
int EnterUserMode(UserCtx_t *ctx);

/* Process control block describes process related resources. */
typedef struct Proc {
  int pid;                 /* process identifier */
  void *ustk;              /* user stack */
  size_t ustksz;           /* size of user stack */
  int exitcode;            /* stores value from exit system call */
  jmp_buf retctx;          /* context restored when process finishes */
  UserCtx_t usrctx;        /* initial user context */
  Hunk_t *hunk;            /* first hunk of executable file */
  File_t *fdtab[MAXFILES]; /* file descriptor table */
} Proc_t;

Proc_t *TaskGetProc(void);
void TaskSetProc(Proc_t *proc);

void ProcInit(Proc_t *proc, size_t ustksz);
void ProcFini(Proc_t *proc);
int ProcLoadImage(Proc_t *proc, File_t *exe);
void ProcFreeImage(Proc_t *proc);
void ProcSetArgv(Proc_t *proc, char *const *argv);
void ProcEnter(Proc_t *proc);
__noreturn void ProcExit(Proc_t *proc, int exitcode);
