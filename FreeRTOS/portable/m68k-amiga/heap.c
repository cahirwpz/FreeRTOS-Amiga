#include <FreeRTOS/FreeRTOS.h>
#include <FreeRTOS/task.h>

#include <cdefs.h>
#include <string.h>
#include <boot.h>

#if (configSUPPORT_DYNAMIC_ALLOCATION == 0)
#error This file must not be used if configSUPPORT_DYNAMIC_ALLOCATION is 0
#endif

#define DEBUG 1

#if DEBUG
#include <stdio.h>
#define debug(fmt, ...) printf("%s: " fmt "\n", __func__, __VA_ARGS__)
#define msg(...) printf(__VA_ARGS__)
#else
#define debug(fmt, ...)
#define msg(...)
#endif

#define assert(...) configASSERT(__VA_ARGS__)

#define ALIGNMENT 16

typedef uintptr_t word_t;

typedef enum {
  FREE = 0,
  USED = 1,
  PREVFREE = 2,
} bt_flags;

typedef struct node {
  struct node *prev;
  struct node *next;
} node_t;

typedef struct arena {
  node_t freelst;     /* guard of free block list */
  word_t *end;        /* first address after the arena */
  word_t *last;       /* last block in the arena */
  uint32_t totalFree; /* total number of free bytes */
  uint32_t minFree;   /* minimum recorded number of free bytes */
  word_t start[] __aligned(ALIGNMENT);     /* first block in the arena */
} arena_t;

#define head (&ar->freelst)

static inline word_t bt_size(word_t *bt) {
  return *bt & ~(USED | PREVFREE);
}

__unused static inline int bt_used(word_t *bt) {
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

#define bt_make(bt, size, flags) ar_bt_make(ar, (bt), (size), (flags))
static inline __always_inline void ar_bt_make(arena_t *ar, word_t *bt,
                                              size_t size, bt_flags flags) {
  word_t val = size | flags;
  *bt = val;
  if ((flags & USED) == 0) {
    word_t *ft = (void *)bt + size - sizeof(word_t);
    *ft = val;
  }
  if ((void *)ar->end == (void *)bt + size)
    ar->last = bt;
}

static inline bt_flags bt_get_prevfree(word_t *bt) {
  return *bt & PREVFREE;
}

static inline void bt_clr_prevfree(word_t *bt) {
  if (bt)
    *bt &= ~PREVFREE;
}

static inline void bt_set_prevfree(word_t *bt) {
  *bt |= PREVFREE;
}

static inline void *bt_payload(word_t *bt) {
  return bt + 1;
}

#define bt_next(bt) ar_bt_next(ar, (bt))
static inline word_t *ar_bt_next(arena_t *ar, word_t *bt) {
  word_t *next = (void *)bt + bt_size(bt);
  return (next < ar->end) ? next : NULL;
}

#define bt_prev(bt) ar_bt_prev(ar, (bt))
static inline word_t *ar_bt_prev(arena_t *ar, word_t *bt) {
  word_t *ft = bt - 1;
  return (ft > ar->start) ? (void *)bt - bt_size(ft) : NULL;
}

/* Useful if pointer compression is implemented */
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
  return (size + sizeof(word_t) + ALIGNMENT - 1) & -ALIGNMENT;
}

#if 1
static word_t *find_fit(arena_t *ar, size_t reqsz) {
  for (node_t *n = n_next(head); n != head; n = n_next(n)) {
    word_t *bt = bt_fromptr(n);
    if (bt_size(bt) >= reqsz)
      return bt;
  }
  return NULL;
}
#else
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

static void ar_init(arena_t *ar, void *end) {
  n_setprev(head, head);
  n_setnext(head, head);
  ar->end = end;
  ar->last = ar->start;
  ar->totalFree = end - (void *)ar->start;
  ar->minFree = end - (void *)ar->start;
}

static void *ar_malloc(arena_t *ar, size_t size) {
  size_t reqsz = blksz(size);
  word_t *bt = find_fit(ar, reqsz);
  if (bt == NULL)
    return NULL;

  /* Mark found block as used. */
  size_t sz = bt_size(bt);
  n_remove(bt);
  bt_make(bt, reqsz, USED);
  /* Split free block if needed. */
  if (sz > reqsz) {
    bt_make(bt_next(bt), sz - reqsz, FREE);
    n_insert(bt_next(bt));
  } else {
    /* Nothing to split? Then previous block is not free anymore! */
    bt_clr_prevfree(bt_next(bt));
  }

  /* Did we run out of memory? */
  void *ptr = bt_payload(bt);
  debug("%s(%p, %lu) = %p", __func__, ar, size, ptr);
  return ptr;
}

static void ar_free(arena_t *ar, void *ptr) {
  debug("%s(%p, %p)", __func__, ptr);

  word_t *bt = bt_fromptr(ptr);

  /* Mark block as free. */
  bt_make(bt, bt_size(bt), bt_get_prevfree(bt));
  debug("bt = %p (size: %u)", bt, bt_size(bt));

  word_t *next = bt_next(bt);
  if (next) {
    if (bt_free(next)) {
      /* Coalesce with next block. */
      n_remove(next);
      bt_make(bt, bt_size(bt) + bt_size(next), bt_get_prevfree(bt));
    } else {
      /* Mark next used block with prevfree flag. */
      bt_set_prevfree(next);
    }
  }

  /* Check if can coalesce with previous block. */
  if (bt_get_prevfree(bt)) {
    word_t *prev = bt_prev(bt);
    n_remove(prev);
    bt_make(prev, bt_size(prev) + bt_size(bt), FREE);
    bt = prev;
  }

  n_insert(bt);
}

static void *ar_realloc(arena_t *ar, void *old_ptr, size_t size) {
  word_t *bt = bt_fromptr(old_ptr);
  size_t reqsz = blksz(size);
  size_t sz = bt_size(bt);

  if (reqsz == sz) {
    /* Same size: nothing to do. */
    return old_ptr;
  }

  if (reqsz < sz) {
    /* Shrink block: split block and free second one. */
    bt_make(bt, reqsz, USED | bt_get_prevfree(bt));
    word_t *next = bt_next(bt);
    bt_make(next, sz - reqsz, USED);
    ar_free(ar, bt_payload(next));
    debug("%s(%p, %ld) = %p", __func__, old_ptr, size, old_ptr);
    return old_ptr;
  }

  /* Expand block */
  word_t *next = bt_next(bt);
  if (next && bt_free(next)) {
    /* Use next free block if it has enough space. */
    size_t nextsz = bt_size(next);
    if (sz + nextsz >= reqsz) {
      n_remove(next);
      bt_make(bt, reqsz, USED | bt_get_prevfree(bt));
      word_t *next = bt_next(bt);
      if (sz + nextsz > reqsz) {
        bt_make(next, sz + nextsz - reqsz, FREE);
        n_insert(next);
      } else {
        bt_clr_prevfree(next);
      }
      debug("realloc(%p, %ld) = %p", old_ptr, size, old_ptr);
      return old_ptr;
    }
  }

  /* Run out of options: let real realloc move the block. */
  return NULL;
}

static void ar_check(arena_t *ar) {
  word_t *bt = ar->start;

  if (bt == NULL)
    return;

  word_t *prev;
  int prevfree = 0;

  msg("--=[ all block list ]=---\n");

  for (; bt != NULL; prev = bt, bt = bt_next(bt)) {
    int flag = !!bt_get_prevfree(bt);
    msg("%p: [%c%c:%d] %c\n", bt, "FU"[bt_used(bt)], " P"[flag], bt_size(bt),
        " *"[bt == ar->last]);
    if (bt_free(bt)) {
      word_t *ft = bt_footer(bt);
      assert(*bt == *ft && "Header and footer do not match!");
      assert(!prevfree && "Free block not coalesced?");
      prevfree = 1;
    } else {
      assert(flag == prevfree && "PREVFREE flag mismatch!");
      prevfree = 0;
    }
  }

  assert(prev == ar->last && "Last block set incorrectly!");

  msg("--=[ free block list start ]=---\n");

  for (node_t *n = n_next(head); n != head; n = n_next(n)) {
    word_t *bt = bt_fromptr(n);
    msg("%p: [%p, %p]\n", n, n_prev(n), n_next(n));
    assert(bt_free(bt));
  }

  msg("--=[ free block list end ]=---\n");
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

void *pvPortMallocChip(size_t xSize) {
  return _pvPortMallocBelow(xSize, MEM_CHIP);
}

void vPortFree(void *p) {
  if (p)
    ar_free(arena_of(p), p);
}

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

void vPortMemCheck(void) {
  for (const MemRegion_t *mr = MemRegions; mr->mr_upper; mr++)
    ar_check(arena(mr));
}

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
    mr->mr_lower = (mr->mr_lower + (ALIGNMENT - 1)) & -ALIGNMENT;
    mr->mr_upper = mr->mr_upper & -ALIGNMENT;
    ar_init(arena(mr), (void *)mr->mr_upper);
  }
}
