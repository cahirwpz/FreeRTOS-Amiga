#pragma once

#include "file.h"

typedef struct Hunk {
  uint32_t size;
  struct Hunk *next;
  uint8_t data[0];
} Hunk_t;

Hunk_t *LoadHunkList(File_t *file);
void FreeHunkList(Hunk_t *hunklist);
