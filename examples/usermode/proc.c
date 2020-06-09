#include <FreeRTOS/FreeRTOS.h>
#include <custom.h>
#include <file.h>
#include <amigahunk.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "proc.h"

#define DEBUG 0

void ProcInit(Proc_t *proc, size_t ustksz) {
  bzero(proc, sizeof(Proc_t));

  /* Align to long word size. */
  ustksz = (ustksz + 3) & -4;
  proc->ustksz = ustksz;
  proc->ustk = malloc(ustksz);
  bzero(proc->ustk, ustksz);
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

#define PUSH(sp, x)                                                            \
  { *(--(sp)) = (uint32_t)(x); }

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
  uint32_t *sp = proc->ustk + proc->ustksz;

  /* TODO: copy argv contents to user stack and create argc and argv.
   * Remember that elements on stack must be long word aligned! */
  (void)argv;
  PUSH(sp, NULL); /* argv = {NULL} */
  PUSH(sp, 0);    /* argc = 0 */

  return EnterUserMode(pc, sp, proc);
}
