#ifndef _PROC_H_
#define _PROC_H_

#include <cdefs.h>

#define UPROC_STKSZ (configMINIMAL_STACK_SIZE * 8)
#define KPROC_STKSZ (configMINIMAL_STACK_SIZE * 2)

typedef struct Hunk Hunk_t;

#define MAXFILES 16

typedef struct Proc {
  void *ustk;              /* user stack */
  size_t ustksz;           /* size of user stack */
  Hunk_t *hunk;            /* first hunk of executable file */
  File_t *fdtab[MAXFILES]; /* file descriptor table */
} Proc_t;

void ProcInit(Proc_t *proc, size_t ustksz);
void ProcFini(Proc_t *proc);

int Execute(Proc_t *proc, File_t *exe, char *const *argv);

/* Low-level internal interface. */
typedef struct ProcFrame {
  Proc_t *proc;
  uint32_t _private[11];
} ProcFrame_t;

int EnterUserMode(void *pc, void *sp, Proc_t *proc);
__noreturn void ExitUserMode(ProcFrame_t *pf, int exitcode);

#endif /* !_PROC_H_ */
