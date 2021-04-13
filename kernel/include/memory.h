#pragma once

#include <sys/types.h>

typedef enum MemFlags {
  MF_ZERO = 1, /* clear out allocated memory */
  MF_CHIP = 2, /* allocate block for use with custom chipset */
} MemFlags_t;

void *MemAlloc(size_t size, MemFlags_t flags);
void MemFree(void *ptr);
void *MemRealloc(void *ptr, size_t size);
void MemCheck(int verbose);
