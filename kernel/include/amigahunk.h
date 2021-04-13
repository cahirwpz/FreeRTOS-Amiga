#pragma once

#include "file.h"

/* AmigaOS executable files are composed of hunks described in
 * http://amiga-dev.wikidot.com/file-format:hunk
 * CODE, DATA & BSS hunks are loaded into memory and stay there
 * for the lifetime of a program. */
typedef struct Hunk {
  uint32_t size;     /* size of hunk in multiples of 4 bytes */
  struct Hunk *next; /* singly linked list of loaded hunks */
  uint8_t data[0];
} Hunk_t;

/* Loads AmigaOS executable from `file`.
 * Performs relocation i.e. processing of RELOC* hunks. */
Hunk_t *LoadHunkList(File_t *file);

/* Removes loaded executable file from memory.
 * `hunklist` is pointer to first loaded hunk. */
void FreeHunkList(Hunk_t *hunklist);
