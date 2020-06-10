#ifndef _PROC_H_
#define _PROC_H_

#include <cdefs.h>
#include <stddef.h>
#include <setjmp.h>

#define TLS_PROC 0

#define UPROC_STKSZ (configMINIMAL_STACK_SIZE * 8)
#define KPROC_STKSZ (configMINIMAL_STACK_SIZE * 2)

typedef struct File File_t;
typedef struct Hunk Hunk_t;

#define MAXFILES 16

typedef struct Proc {
  int pid;                 /* process identifier */
  void *ustk;              /* user stack */
  size_t ustksz;           /* size of user stack */
  int exitcode;            /* stores value from exit system call */
  jmp_buf retctx;          /* context restored when process finishes */
  Hunk_t *hunk;            /* first hunk of executable file */
  File_t *fdtab[MAXFILES]; /* file descriptor table */
} Proc_t;

Proc_t *TaskGetProc(void);
void TaskSetProc(Proc_t *proc);

void ProcInit(Proc_t *proc, size_t ustksz);
void ProcFini(Proc_t *proc);
int ProcLoadImage(Proc_t *proc, File_t *exe);
void ProcFreeImage(Proc_t *proc);
void ProcExecute(Proc_t *proc, char *const *argv);

/* Descriptor table management procedures. */
int ProcFileInstall(Proc_t *proc, int fd, File_t *file);

/* Low-level internal interface. */
int EnterUserMode(void *pc, void *sp);
__noreturn void ExitUserMode(Proc_t *proc, int exitcode);

#endif /* !_PROC_H_ */
