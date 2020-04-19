#include <FreeRTOS/FreeRTOS.h>
#include <amigahunk.h>
#include <strings.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define DEBUG 0

#define HUNK_CODE 1001
#define HUNK_DATA 1002
#define HUNK_BSS 1003
#define HUNK_RELOC32 1004
#define HUNK_SYMBOL 1008
#define HUNK_DEBUG 1009
#define HUNK_END 1010
#define HUNK_HEADER 1011

#define HUNKF_CHIP BIT(30)
#define HUNKF_FAST BIT(31)

static long ReadLong(File_t *fh) {
  long v = 0;
  FileRead(fh, &v, sizeof(v));
  return v;
}

static void SkipLongs(File_t *fh, int n) {
  FileSeek(fh, n * sizeof(int), SEEK_CUR);
}

static bool AllocHunks(File_t *fh, Hunk_t **hunkArray, short hunkCount) {
  Hunk_t *prev = NULL;

  do {
    /* size specifiers including memory attribute flags */
    uint32_t n = ReadLong(fh);

    Hunk_t *hunk = ((n & HUNKF_CHIP) ? pvPortMallocChip : pvPortMalloc)
      (sizeof(Hunk_t) + n * sizeof(int));
    *hunkArray++ = hunk;

    if (!hunk)
      return false;

    hunk->size = n * sizeof(int);
    hunk->next = NULL;
    bzero(hunk->data, n * sizeof(int));

    if (prev)
      prev->next = hunk;

    prev = hunk;
  } while (--hunkCount);

  return true;
}

static bool LoadHunks(File_t *fh, Hunk_t **hunkArray) {
  int hunkIndex = 0;
  Hunk_t *hunk = hunkArray[hunkIndex++];
  short hunkId;
  bool hunkRoot = true;

  while ((hunkId = ReadLong(fh))) {
    int n;

    if (hunkId == HUNK_CODE || hunkId == HUNK_DATA || hunkId == HUNK_BSS) {
      hunkRoot = true;
      n = ReadLong(fh);
      if (hunkId != HUNK_BSS)
        FileRead(fh, hunk->data, n * sizeof(int));
#if DEBUG
      {
        const char *hunkType;

        if (hunkId == HUNK_CODE)
          hunkType = "CODE";
        else if (hunkId == HUNK_DATA)
          hunkType = "DATA";
        else
          hunkType = " BSS";

        printf("%s: %p - %p\n", hunkType, hunk->data, hunk->data + hunk->size);
      }
#endif
    } else if (hunkId == HUNK_DEBUG) {
      n = ReadLong(fh);
      SkipLongs(fh, n);
    } else if (hunkId == HUNK_RELOC32) {
      while ((n = ReadLong(fh))) {
        int hunkNum = ReadLong(fh);
        int32_t hunkRef = (int32_t)hunkArray[hunkNum]->data;
        do {
          int hunkOff = ReadLong(fh);
          int32_t *ptr = (void *)hunk->data + hunkOff;
          *ptr += hunkRef;
        } while (--n);
      }
    } else if (hunkId == HUNK_SYMBOL) {
      while ((n = ReadLong(fh))) 
        SkipLongs(fh, n + 1);
    } else if (hunkId == HUNK_END) {
      if (hunkRoot) {
        hunkRoot = false;
        hunk = hunkArray[hunkIndex++];
      }
    } else {
#if DEBUG
      printf("Unknown hunk $%04x!\n", hunkCode);
#endif
      return false;
    }
  }

  return true;
}

Hunk_t *LoadHunkList(File_t *fh) {
  short hunkId = ReadLong(fh);

  if (hunkId != HUNK_HEADER)
    return NULL;

  /* Skip resident library names. */
  int n;
  while ((n = ReadLong(fh)))
    SkipLongs(fh, n);

  /*
   * number of hunks (including resident libraries and overlay hunks)
   * number of the first (root) hunk
   * number of the last (root) hunk
   */
  SkipLongs(fh, 1);
  int first = ReadLong(fh);
  int last = ReadLong(fh);

  int hunkCount = last - first + 1;
  Hunk_t **hunkArray = alloca(sizeof(Hunk_t *) * hunkCount);

  if (AllocHunks(fh, hunkArray, hunkCount))
    if (LoadHunks(fh, hunkArray))
      return hunkArray[0];

  FreeHunkList(hunkArray[0]);
  return NULL;
}

void FreeHunkList(Hunk_t *hunk) {
  do {
    Hunk_t *next = hunk->next;
    vPortFree(hunk);
    hunk = next;
  } while (hunk);
}
