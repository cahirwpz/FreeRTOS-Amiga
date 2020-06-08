#include <file.h>
#include "usermode.h"

__noreturn void EnterUserMode(uintptr_t pc, uintptr_t sp);

__noreturn void RunProgram(File_t *exe) {
  Hunk_t *first = LoadHunkList(exe);
  FileClose(exe);

  for (Hunk_t *hunk = first; hunk; hunk = hunk->next)
    hexdump(hunk->data, hunk->size);

  /* Assume first hunk is executable. */
  void (*_start)(void) = (void *)first->data;

  /* Call _start procedure which is assumed to be first. */
  _start();
}
