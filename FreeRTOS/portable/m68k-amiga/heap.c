#include <FreeRTOS.h>
#include <task.h>
#include <boot.h>

#if (configSUPPORT_DYNAMIC_ALLOCATION == 0)
#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

typedef struct block block_t;
typedef struct block *block_p;
typedef struct memory *memory_t;

/* The code assumes block & memory structures begin with a pointer to block!
 * There's no valid block with {next = DEAD_BLOCK} and {size > 0} ! */

struct block {
  block_p next; /* points to next free block, otherwise set do DEAD_BLOCK */
  int32_t size; /* real size without header; size < 0 => used, otherwise free */
  char data[];
};

struct memory {
  block_p firstFree;  /* pointer to first free block */
  uint32_t totalFree; /* total number of free bytes */
  uint32_t minFree;   /* minimum recorded number of free bytes */
  uintptr_t final;
  block_t first[];
};

#define DEAD_BLOCK (void *)0xDEADC0DE
#define BLOCK_SIZE sizeof(block_t)
#define BLOCK_OF(p) (block_t *)((uintptr_t)(p)-BLOCK_SIZE)

#define ALIGN(x, n) (((x) + (n)-1) & -(n))

static void *_pvPortMalloc(int size, memory_t m) {
  void *ptr = NULL;

  /* Loose up to (BLOCK_SIZE - 1) bytes due to internal fragmentation. */
  size = ALIGN(size, BLOCK_SIZE);

  /* Enter critical section with preemption turned off. */
  vTaskSuspendAll();
  {
    /* Pointer trick for linked list to reduce number of cases to handle. */
    block_p *blk_p = &m->firstFree;

    /* Find a block on free list that is large enough. */
    while (*blk_p != NULL) {
      block_p curr = *blk_p;
      /* Is this block big enough? */
      if (curr->size >= size) {
        /* If it's too big then split it. */
        if (curr->size > size) {
          /* Create a block just after current one finishes. */
          block_p succ = (block_p)(curr->data + size);
          /* Calculate leftover size minus block header size. */
          succ->size = curr->size - size - BLOCK_SIZE;
          /* Insert the block on free list. */
          succ->next = curr->next;
          curr->next = succ;
        }
        /* Decrease the amount of available memory. */
        m->totalFree -= size + BLOCK_SIZE;
        /* Record the lowest amount of available memory. */
        if (m->totalFree < m->minFree)
          m->minFree = m->totalFree;
        /* Previous free block points to free successor block. */
        *blk_p = curr->next;
        /* Mark block as used and set up canary. */
        curr->next = DEAD_BLOCK;
        curr->size = -curr->size;
        ptr = curr->data;
        break;
      }
      blk_p = &curr->next;
    }
  }
  xTaskResumeAll();

  return ptr;
}

static void _vPortFree(void *p, memory_t m) {
  /* Pointer to block to be freed. */
  block_p todo = BLOCK_OF(p);
  /* Is used block constructed as we expect? */
  configASSERT(todo->next == DEAD_BLOCK && todo->size < 0);

  /* Enter critical section with preemption turned off. */
  vTaskSuspendAll();
  {
    /* Mark block as free */
    todo->size = -todo->size;
    /* Record amount of freed memory. */
    size_t freed = todo->size;

    /* Find the place on free list for the block to be inserted. */
    block_p *blk_p = &m->firstFree;
    while (*blk_p != NULL) {
      block_p succ = *blk_p;
      /* The block must fit between two consecutive free block with addresses
       * correspondingly lower and higher to freed block. */
      if ((void *)blk_p < (void *)todo && todo < succ)
        break;
      blk_p = &succ->next;
    }

    /* Insert the block onto free block list. */
    todo->next = *blk_p;
    *blk_p = todo;

    /* Successor block, that is adjacent to freed block. The pointer may be
     * invalid, i.e. reference to address after the end of memory range. */
    block_p succ = (block_p)(todo->data + todo->size);
    /* Try to merge with the block to the right. Is the successor block
     * (a) a block within memory range (b) a free block? */
    if ((void *)succ < (void *)m->final && todo->next == succ) {
      /* Merge successor block with freed one. */
      todo->size += succ->size + BLOCK_SIZE;
      todo->next = succ->next;
      /* Mark merged block as dead. */
      succ->next = DEAD_BLOCK;
      /* Gained extra BLOCK_SIZE bytes! */
      freed += BLOCK_SIZE;
    }

    /* Predecessor free block. The pointer may be invalid, i.e. reference to
     * address before the beginning of memory range. */
    block_p pred = BLOCK_OF(*blk_p);
    /* Try to merge with the block to the left. Is the predecessor block:
     * (a) a block within memory range (b) adjacent to freed block? */
    if ((void *)pred > (void *)m && pred->next == todo) {
      /* Merge freed block with predecessor one. */
      pred->size += todo->size + BLOCK_SIZE;
      pred->next = todo->next;
      /* Mark merged block as dead. */
      todo->next = DEAD_BLOCK;
      /* Gained extra BLOCK_SIZE bytes! */
      freed += BLOCK_SIZE;
    }

    /* Increase the amount of available memory. */
    m->totalFree += freed;
  }
  xTaskResumeAll();
}

const MemRegion_t *MemRegions;

void *_pvPortMallocBelow(size_t xSize, uintptr_t xUpperAddr) {
  void *ptr;

  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++) {
    memory_t m = (memory_t)mr->mr_lower;
    if (((uintptr_t)m < xUpperAddr) && (ptr = _pvPortMalloc(xSize, m)))
      return ptr;
  }

#if (configUSE_MALLOC_FAILED_HOOK == 1)
  extern void vApplicationMallocFailedHook(void);
  vApplicationMallocFailedHook();
#endif
  return NULL;
}

/* There's no real Amiga that has more than 2MiB of chip memory. */
#define MEM_ANY (-1U)
#define MEM_CHIP (1U << 21)

void *pvPortMalloc(size_t xSize) {
  return _pvPortMallocBelow(xSize, MEM_ANY);
}

void *pvPortMallocChip(size_t xSize) {
  return _pvPortMallocBelow(xSize, MEM_CHIP);
}

void vPortFree(void *p) {
  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++) {
    uintptr_t start = mr->mr_lower;
    uintptr_t end = mr->mr_upper;
    if ((uintptr_t)p > start && (uintptr_t)p < end) {
      _vPortFree(p, (memory_t)mr->mr_lower);
      return;
    }
  }

  configASSERT(p == NULL);
}

size_t xPortGetFreeHeapSize(void) {
  size_t sum = 0;
  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++) {
    memory_t m = (memory_t)mr->mr_lower;
    sum += m->totalFree;
  }
  return sum;
}

size_t xPortGetMinimumEverFreeHeapSize(void) {
  size_t sum = 0;
  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++) {
    memory_t m = (memory_t)mr->mr_lower;
    sum += m->minFree;
  }
  return sum;
}

void vPortDefineMemoryRegions(MemRegion_t *aMemRegions) {
  MemRegions = aMemRegions;

  for (MemRegion_t *mr = aMemRegions; mr->mr_upper; mr++) {
    /* align upper and lower addresses to BLOCK_SIZE boundary */
    mr->mr_lower = (mr->mr_lower + (BLOCK_SIZE - 1)) & -BLOCK_SIZE;
    mr->mr_upper = mr->mr_upper & -BLOCK_SIZE;

    memory_t m = (memory_t)mr->mr_lower;

    size_t real_size = (char *)mr->mr_upper - (char *)m->first->data;

    m->totalFree = real_size;
    m->minFree = real_size;
    m->final = mr->mr_upper;
    m->firstFree = m->first;
    m->first->next = NULL;
    m->first->size = real_size;
  }
}
