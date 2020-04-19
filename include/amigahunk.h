#ifndef _AMIGAHUNK_H_
#define _AMIGAHUNK_H_

#include "file.h"

typedef struct Hunk {
  uint32_t size;
  struct Hunk *next;
  uint8_t data[0];
} Hunk_t;

Hunk_t *LoadHunkList(File_t *file);
void FreeHunkList(Hunk_t *hunklist);

#endif /* !__AMIGAHUNK__ */
