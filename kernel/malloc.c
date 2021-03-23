#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <sys/cdefs.h>
#include <limits.h>
#include <string.h>
#include <libkern.h>
#include <boot.h>

#if (configSUPPORT_DYNAMIC_ALLOCATION == 0)
#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

#define DEBUG 0

#if DEBUG
#define debug(fmt, ...) kprintf("%s: " fmt "\n", __func__, __VA_ARGS__)
#else
#define debug(fmt, ...)
#endif

#define assert(...) configASSERT(__VA_ARGS__)

typedef uintptr_t word_t;

#define ALIGNMENT 16
#define CANARY 0xDEADC0DE

/* Free block consists of header BT, pointer to previous and next free block,
 * payload and footer BT. */
#define FREEBLK_SZ (4 * sizeof(word_t))
/* Used block consists of header BT, user memory and canary. */
#define USEDBLK_SZ (2 * sizeof(word_t))

/* Boundary tag flags. */
typedef enum {
  FREE = 0,     /* this block is free */
  USED = 1,     /* this block is used */
  PREVFREE = 2, /* previous block is free */
  ISLAST = 4,   /* last block in an arena */
} bt_flags;

/* Stored in payload of free blocks. */
typedef struct node {
  struct node *prev;
  struct node *next;
} node_t;

/* Structure kept in the header of each managed memory region. */
typedef struct arena {
  node_t freelst;     /* guard of free block list */
  word_t *end;        /* first address after the arena */
  uint32_t totalFree; /* total number of free bytes */
  uint32_t minFree;   /* minimum recorded number of free bytes */
  uint32_t pad[2];    /* make user address aligned to ALIGNMENT */
  word_t start[];     /* first block in the arena */
} arena_t;

#define head (&ar->freelst)

static inline word_t bt_size(word_t *bt) {
  return *bt & ~(USED | PREVFREE | ISLAST);
}

static inline int bt_used(word_t *bt) {
  return *bt & USED;
}

static inline int bt_free(word_t *bt) {
  return !(*bt & USED);
}

static inline word_t *bt_footer(word_t *bt) {
  return (void *)bt + bt_size(bt) - sizeof(word_t);
}

static inline word_t *bt_fromptr(void *ptr) {
  return (word_t *)ptr - 1;
}

static inline __always_inline void bt_make(word_t *bt, size_t size,
                                           bt_flags flags) {
  word_t val = size | flags;
  *bt = val;
  word_t *ft = (void *)bt + size - sizeof(word_t);
  *ft = (flags & USED) ? CANARY : val;
}

static inline int bt_has_canary(word_t *bt) {
  return *bt_footer(bt) == CANARY;
}

static inline bt_flags bt_get_flags(word_t *bt) {
  return *bt & (PREVFREE | ISLAST);
}

static inline bt_flags bt_get_prevfree(word_t *bt) {
  return *bt & PREVFREE;
}

static inline bt_flags bt_get_islast(word_t *bt) {
  return *bt & ISLAST;
}

static inline void bt_clr_islast(word_t *bt) {
  *bt &= ~ISLAST;
}

static inline void bt_clr_prevfree(word_t *bt) {
  *bt &= ~PREVFREE;
}

static inline void bt_set_prevfree(word_t *bt) {
  *bt |= PREVFREE;
}

static inline void *bt_payload(word_t *bt) {
  return bt + 1;
}

static inline word_t *bt_next(word_t *bt) {
  return (void *)bt + bt_size(bt);
}

/* Must never be called on first block in an arena. */
static inline word_t *bt_prev(word_t *bt) {
  word_t *ft = bt - 1;
  return (void *)bt - bt_size(ft);
}

/* These are only useful if node_t pointers are compressed. */
static inline node_t *n_prev(node_t *node) {
  return node->prev;
}

static inline node_t *n_next(node_t *node) {
  return node->next;
}

static inline void n_setprev(node_t *node, node_t *prev) {
  node->prev = prev;
}

static inline void n_setnext(node_t *node, node_t *next) {
  node->next = next;
}

#define n_insert(bt) ar_n_insert(ar, (bt))
static inline void ar_n_insert(arena_t *ar, word_t *bt) {
  node_t *node = bt_payload(bt);
  node_t *prev = n_prev(head);

  /* Insert at the end of free block list. */
  n_setnext(node, head);
  n_setprev(node, prev);
  n_setnext(prev, node);
  n_setprev(head, node);
}

static inline void n_remove(word_t *bt) {
  node_t *node = bt_payload(bt);
  node_t *prev = n_prev(node);
  node_t *next = n_next(node);
  n_setnext(prev, next);
  n_setprev(next, prev);
}

static inline size_t blksz(size_t size) {
  return roundup(size + USEDBLK_SZ, ALIGNMENT);
}

#if 1
/* First fit */
static word_t *find_fit(arena_t *ar, size_t reqsz) {
  for (node_t *n = n_next(head); n != head; n = n_next(n)) {
    word_t *bt = bt_fromptr(n);
    if (bt_size(bt) >= reqsz)
      return bt;
  }
  return NULL;
}
#else
/* Best fit */
static word_t *find_fit(arena_t *ar, size_t reqsz) {
  word_t *best = NULL;
  size_t bestsz = INT_MAX;

  for (node_t *n = n_next(head); n != head; n = n_next(n)) {
    word_t *bt = bt_fromptr(n);
    size_t sz = bt_size(bt);
    if (sz == reqsz)
      return bt;
    if (sz > reqsz && sz < bestsz) {
      best = bt;
      bestsz = sz;
    }
  }

  return best;
}
#endif

static inline void ar_dec_free(arena_t *ar, size_t sz) {
  /* Decrease the amount of available memory. */
  ar->totalFree -= sz;
  /* Record the lowest amount of available memory. */
  if (ar->totalFree < ar->minFree)
    ar->minFree = ar->totalFree;
}

static void ar_init(arena_t *ar, void *end) {
  size_t sz = end - (void *)ar->start;

  n_setprev(head, head);
  n_setnext(head, head);
  ar->end = end;
  ar->totalFree = sz - USEDBLK_SZ;
  ar->minFree = INT_MAX;
  word_t *bt = ar->start;
  bt_make(bt, sz, FREE | ISLAST);
  n_insert(bt);
}

static void *ar_malloc(arena_t *ar, size_t size) {
  size_t reqsz = blksz(size);

  vTaskSuspendAll();

  word_t *bt = find_fit(ar, reqsz);
  if (bt != NULL) {
    bt_flags is_last = bt_get_islast(bt);
    size_t memsz = reqsz - USEDBLK_SZ;
    /* Mark found block as used. */
    size_t sz = bt_size(bt);
    n_remove(bt);
    bt_make(bt, reqsz, USED | is_last);
    /* Split free block if needed. */
    word_t *next = bt_next(bt);
    if (sz > reqsz) {
      bt_make(next, sz - reqsz, FREE | is_last);
      bt_clr_islast(bt);
      memsz += USEDBLK_SZ;
      n_insert(next);
    } else if (!is_last) {
      /* Nothing to split? Then previous block is not free anymore! */
      bt_clr_prevfree(next);
    }
    ar_dec_free(ar, memsz);
  }

  xTaskResumeAll();

  /* Did we run out of memory? */
  void *ptr = bt ? bt_payload(bt) : NULL;
  debug("%s(%p, %lu) = %p", __func__, ar, size, ptr);
  return ptr;
}

static void ar_free(arena_t *ar, void *ptr) {
  debug("%s(%p, %p)", __func__, ar, ptr);

  word_t *bt = bt_fromptr(ptr);

  vTaskSuspendAll();

  assert(bt_used(bt) && bt_has_canary(bt)); /* Is block free and has canary? */

  /* Mark block as free. */
  size_t memsz = bt_size(bt) - USEDBLK_SZ;
  size_t sz = bt_size(bt);
  bt_make(bt, sz, FREE | bt_get_flags(bt));
  debug("bt = %p (size: %u)", bt, sz);

  word_t *next = bt_next(bt);
  if (next) {
    if (bt_free(next)) {
      /* Coalesce with next block. */
      n_remove(next);
      sz += bt_size(next);
      bt_make(bt, sz, FREE | bt_get_prevfree(bt) | bt_get_islast(next));
      memsz += USEDBLK_SZ;
    } else {
      /* Mark next used block with prevfree flag. */
      bt_set_prevfree(next);
    }
  }

  /* Check if can coalesce with previous block. */
  if (bt_get_prevfree(bt)) {
    word_t *prev = bt_prev(bt);
    n_remove(prev);
    sz += bt_size(prev);
    bt_make(prev, sz, FREE | bt_get_islast(bt));
    memsz += USEDBLK_SZ;
    bt = prev;
  }

  ar->totalFree += memsz;
  n_insert(bt);

  xTaskResumeAll();
}

static void *ar_realloc(arena_t *ar, void *old_ptr, size_t size) {
  void *new_ptr = NULL;
  word_t *bt = bt_fromptr(old_ptr);
  size_t reqsz = blksz(size);
  size_t sz = bt_size(bt);

  if (reqsz == sz) {
    /* Same size: nothing to do. */
    return old_ptr;
  }

  vTaskSuspendAll();

  if (reqsz < sz) {
    bt_flags is_last = bt_get_islast(bt);
    /* Shrink block: split block and free second one. */
    bt_make(bt, reqsz, USED | bt_get_prevfree(bt));
    word_t *next = bt_next(bt);
    bt_make(next, sz - reqsz, USED | is_last);
    ar_free(ar, bt_payload(next));
    new_ptr = old_ptr;
  } else {
    /* Expand block */
    word_t *next = bt_next(bt);
    if (next && bt_free(next)) {
      /* Use next free block if it has enough space. */
      bt_flags is_last = bt_get_islast(next);
      size_t nextsz = bt_size(next);
      if (sz + nextsz >= reqsz) {
        size_t memsz;
        n_remove(next);
        bt_make(bt, reqsz, USED | bt_get_prevfree(bt));
        word_t *next = bt_next(bt);
        if (sz + nextsz > reqsz) {
          bt_make(next, sz + nextsz - reqsz, FREE | is_last);
          memsz = reqsz - sz;
          n_insert(next);
        } else {
          memsz = nextsz - USEDBLK_SZ;
          bt_clr_prevfree(next);
        }
        ar_dec_free(ar, memsz);
        new_ptr = old_ptr;
      }
    }
  }

  xTaskResumeAll();

  debug("%s(%p, %ld) = %p", __func__, old_ptr, size, new_ptr);
  return new_ptr;
}

#define msg(...)                                                               \
  if (verbose) {                                                               \
    klog(__VA_ARGS__);                                                         \
  }

static void ar_check(arena_t *ar, int verbose) {
  word_t *bt = ar->start;

  vTaskSuspendAll();

  word_t *prev = NULL;
  int prevfree = 0;
  unsigned freeMem = 0, dangling = 0;

  msg("--=[ all block list ]=---\n");

  for (; bt < ar->end; prev = bt, bt = bt_next(bt)) {
    int flag = !!bt_get_prevfree(bt);
    int is_last = !!bt_get_islast(bt);
    msg("%p: [%c%c:%d] %c\n", bt, "FU"[bt_used(bt)], " P"[flag], bt_size(bt),
        " *"[is_last]);
    if (bt_free(bt)) {
      word_t *ft = bt_footer(bt);
      assert(*bt == *ft); /* Header and footer do not match? */
      assert(!prevfree);  /* Free block not coalesced? */
      prevfree = 1;
      freeMem += bt_size(bt) - USEDBLK_SZ;
      dangling++;
    } else {
      assert(flag == prevfree);  /* PREVFREE flag mismatch? */
      assert(bt_has_canary(bt)); /* Canary damaged? */
      prevfree = 0;
    }
  }

  assert(bt_get_islast(prev));      /* Last block set incorrectly? */
  assert(freeMem == ar->totalFree); /* Total free memory miscalculated? */

  msg("--=[ free block list start ]=---\n");

  for (node_t *n = n_next(head); n != head; n = n_next(n)) {
    word_t *bt = bt_fromptr(n);
    msg("%p: [%p, %p]\n", n, n_prev(n), n_next(n));
    assert(bt_free(bt));
    dangling--;
  }

  msg("--=[ free block list end ]=---\n");

  assert(dangling == 0 && "Dangling free blocks!");

  xTaskResumeAll();
}

/* --=[ wrappers ]=---------------------------------------------------------- */

const MemRegion_t *MemRegions;

static inline arena_t *arena(const MemRegion_t *mr) {
  return (arena_t *)mr->mr_lower;
}

static int inside(void *ptr, arena_t *ar) {
  return ptr >= (void *)ar->start && ptr < (void *)ar->end;
}

static arena_t *arena_of(void *p) {
  arena_t *ar = NULL;
  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++) {
    if (inside(p, arena(mr))) {
      ar = arena(mr);
      break;
    }
  }
  assert(ar != NULL);
  return ar;
}

void *_pvPortMallocBelow(size_t xSize, uintptr_t xUpperAddr) {
  void *ptr;
  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++) {
    if ((uintptr_t)arena(mr) >= xUpperAddr)
      continue;
    if ((ptr = ar_malloc(arena(mr), xSize)))
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

__strong_alias(kmalloc, pvPortMalloc);

void *pvPortMallocChip(size_t xSize) {
  return _pvPortMallocBelow(xSize, MEM_CHIP);
}

void vPortFree(void *p) {
  if (p != NULL)
    ar_free(arena_of(p), p);
}

__strong_alias(kfree, vPortFree);

void *pvPortCalloc(size_t nmemb, size_t size) {
  size_t bytes = nmemb * size;
  void *new_ptr = pvPortMalloc(bytes);
  if (new_ptr)
    memset(new_ptr, 0, bytes);
  return new_ptr;
}

__strong_alias(kcalloc, pvPortCalloc);

void *pvPortRealloc(void *old_ptr, size_t size) {
  void *new_ptr;

  if (size == 0) {
    vPortFree(old_ptr);
    return NULL;
  }

  if (old_ptr == NULL)
    return pvPortMalloc(size);

  if ((new_ptr = ar_realloc(arena_of(old_ptr), old_ptr, size)))
    return new_ptr;

  /* Run out of options - need to move block physically. */
  if ((new_ptr = pvPortMalloc(size))) {
    word_t *bt = bt_fromptr(old_ptr);
    debug("%s(%p, %ld) = %p", __func__, old_ptr, size, new_ptr);
    memcpy(new_ptr, old_ptr, bt_size(bt) - sizeof(word_t));
    vPortFree(old_ptr);
    return new_ptr;
  }

  return NULL;
}

__strong_alias(krealloc, pvPortRealloc);

void vPortMemCheck(int verbose) {
  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++)
    ar_check(arena(mr), verbose);
}

__strong_alias(kmcheck, vPortMemCheck);

size_t xPortGetFreeHeapSize(void) {
  size_t sum = 0;
  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++)
    sum += arena(mr)->totalFree;
  return sum;
}

size_t xPortGetMinimumEverFreeHeapSize(void) {
  size_t sum = 0;
  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++)
    sum += arena(mr)->minFree;
  return sum;
}

void vPortDefineMemoryRegions(MemRegion_t *aMemRegions) {
  MemRegions = aMemRegions;

  for (MemRegion_t *mr = aMemRegions; mr->mr_upper; mr++) {
    /* align upper and lower addresses */
    mr->mr_lower = roundup(mr->mr_lower, ALIGNMENT);
    mr->mr_upper = rounddown(mr->mr_upper, ALIGNMENT) - sizeof(word_t);
    ar_init(arena(mr), (void *)mr->mr_upper);
  }
}
