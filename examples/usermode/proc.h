#ifndef _PROC_H_
#define _PROC_H_

#include <cdefs.h>
#include <setjmp.h>

#define TLS_PROC 0

#define UPROC_STKSZ (configMINIMAL_STACK_SIZE * 8)
#define KPROC_STKSZ (configMINIMAL_STACK_SIZE * 2)

typedef struct Hunk Hunk_t;

#define MAXFILES 16

typedef struct Proc {
  void *ustk;              /* user stack */
  size_t ustksz;           /* size of user stack */
  int exitcode;            /* stores value from exit system call */
  jmp_buf retctx;          /* context restored when process finishes */
  Hunk_t *hunk;            /* first hunk of executable file */
  File_t *fdtab[MAXFILES]; /* file descriptor table */
} Proc_t;

Proc_t *GetCurrentProc(void);

void ProcInit(Proc_t *proc, size_t ustksz);
void ProcFini(Proc_t *proc);

int Execute(Proc_t *proc, File_t *exe, char *const *argv);

/* Low-level internal interface. */
int EnterUserMode(void *pc, void *sp);

#endif /* !_PROC_H_ */
