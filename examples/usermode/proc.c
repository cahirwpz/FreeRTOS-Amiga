#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>
#include <custom.h>
#include <file.h>
#include <amigahunk.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include "proc.h"

#define DEBUG 0

Proc_t *GetCurrentProc(void) {
  return pvTaskGetThreadLocalStoragePointer(NULL, TLS_PROC);
}

void ProcInit(Proc_t *proc, size_t ustksz) {
  bzero(proc, sizeof(Proc_t));

  /* Align to long word size. */
  ustksz = (ustksz + 3) & -4;
  proc->ustksz = ustksz;
  proc->ustk = malloc(ustksz);
  bzero(proc->ustk, ustksz);

  vTaskSetThreadLocalStoragePointer(NULL, TLS_PROC, (void *)proc);
}

void ProcFini(Proc_t *proc) {
  Hunk_t *next;
  for (Hunk_t *hunk = proc->hunk; hunk != NULL; hunk = next) {
    next = hunk->next;
    free(hunk);
  }

  for (int i = 0; i < MAXFILES; i++) {
    File_t *f = proc->fdtab[i];
    if (f != NULL)
      FileClose(f);
  }

  free(proc->ustk);
}

#define PUSH(sp, v)                                                            \
  {                                                                            \
    uint32_t *__sp = sp;                                                       \
    *(--__sp) = (uint32_t)v;                                                   \
    sp = __sp;                                                                 \
  }

static void *CopyArgVec(void *sp, char *const *argv) {
  /* Copy argv contents to user stack and create argc and argv. */
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
  return sp;
}

int Execute(Proc_t *proc, File_t *exe, char *const *argv) {
  Hunk_t *hunk = LoadHunkList(exe);
  FileClose(exe);

  if (hunk == NULL)
    return -1;

  proc->hunk = hunk;

#if DEBUG
  for (Hunk_t *h = hunk; h != NULL; h = h->next)
    hexdump(h->data, h->size);
#endif

  /* Execute assumes that _start procedure is placed
   * at the beginning of first hunk of executable file. */
  void *pc = hunk->data;
  /* Stack grows down. */
  void *sp = CopyArgVec(proc->ustk + proc->ustksz, argv);

  if (!setjmp(proc->retctx))
    EnterUserMode(pc, sp);
  return 0;
}
