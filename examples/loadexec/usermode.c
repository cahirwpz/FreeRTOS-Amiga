#include <FreeRTOS/FreeRTOS.h>
#include <custom.h>
#include <file.h>
#include <amigahunk.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include "usermode.h"

#define DEBUG 0

int RunProgram(File_t *exe, size_t ustksz) {
  Hunk_t *first = LoadHunkList(exe);
  FileClose(exe);

#if DEBUG
  for (Hunk_t *hunk = first; hunk != NULL; hunk = hunk->next)
    hexdump(hunk->data, hunk->size);
#endif

  /* Align to long word size. */
  ustksz = (ustksz + 3) & -4;
  void *sp = malloc(ustksz);
  bzero(sp, ustksz);

  /* Assume _start procedure is placed at the beginning of first hunk,
   * which happens to be executable. */
  int rv = EnterUserMode(first->data, sp + ustksz);

  /* Free stack and hunks. */
  free(sp);

  Hunk_t *next;
  for (Hunk_t *hunk = first; hunk != NULL; hunk = next) {
    next = hunk->next;
    free(hunk);
  }

  return rv;
}
