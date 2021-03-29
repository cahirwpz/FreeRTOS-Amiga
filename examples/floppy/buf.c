/* Number of lists containing buffered blocks. */
#define NBUCKETS 16

/* Since majority of files in a filesystem are small, `idx` values will be
 * usually low. Since ext2fs tends to allocate blocks at the beginning of each
 * block group, `ino` values are less predictable. */
#define BUCKET(ino, idx) (((ino) + (idx)) % NBUCKETS)

/* That should give us around 64kB worth of buffers. */
#define NBLOCKS (NBUCKETS * 4)

/* Structure that is used to manage buffer of single block. */
typedef struct blk {
  TAILQ_ENTRY(blk) b_hash;
  TAILQ_ENTRY(blk) b_link;
  uint32_t b_blkaddr; /* block address on the block device */
  uint32_t b_inode;   /* i-node number of file this buffer refers to */
  uint32_t b_index;   /* block index from the beginning of file */
  uint32_t b_refcnt;  /* if zero then block can be reused */
  void *b_data;       /* raw data from this buffer */
} blk_t;

typedef TAILQ_HEAD(blk_list, blk) blk_list_t;

/* BLK_ZERO is a special value that reflect the fact that block 0 may be used to
 * represent a block filled with zeros. You must not dereference the value! */
#define BLK_ZERO ((blk_t *)-1L)

/* All memory for buffers and buffer management is allocated statically.
 * Using malloc for these would introduce unnecessary complexity. */
static alignas(BLKSIZE) char blkdata[NBLOCKS][BLKSIZE];
static blk_t blocks[NBLOCKS];
static blk_list_t buckets[NBUCKETS]; /* all blocks with valid data */
static blk_list_t lrulst;            /* free blocks with valid data */
static blk_list_t freelst;           /* free blocks that are empty */

/*
 * Buffering routines.
 */

/* Opens filesystem image file and initializes block buffers. */
static int blk_init(const char *fspath) {
  if ((fd_ext2 = open(fspath, O_RDWR)) < 0)
    return errno;

  /* Initialize list structures. */
  TAILQ_INIT(&lrulst);
  TAILQ_INIT(&freelst);
  for (int i = 0; i < NBUCKETS; i++)
    TAILQ_INIT(&buckets[i]);

  /* Initialize all blocks and put them on free list. */
  for (int i = 0; i < NBLOCKS; i++) {
    blocks[i].b_data = blkdata[i];
    TAILQ_INSERT_TAIL(&freelst, &blocks[i], b_link);
  }

  return 0;
}

/* Allocates new block buffer. */
static blk_t *blk_alloc(void) {
  blk_t *blk = NULL;

  /* Initially every empty block is on free list. */
  if (!TAILQ_EMPTY(&freelst)) {
#ifndef STUDENT
    blk = TAILQ_FIRST(&freelst);
    TAILQ_REMOVE(&freelst, blk, b_link);
#endif /* STUDENT */
    return blk;
  }

  /* Eventually free list will become exhausted.
   * Then we'll take the last recently used entry from LRU list. */
  if (!TAILQ_EMPTY(&lrulst)) {
#ifndef STUDENT
    blk = TAILQ_LAST(&lrulst, blk_list);
    TAILQ_REMOVE(&lrulst, blk, b_link);
    blk_list_t *bucket = &buckets[BUCKET(blk->b_inode, blk->b_index)];
    TAILQ_REMOVE(bucket, blk, b_hash);
#endif /* STUDENT */
    return blk;
  }

  /* No buffers!? Have you forgot to release some? */
  panic("Free buffers pool exhausted!");
}

/* Acquires a block buffer for file identified by `ino` i-node and block index
 * `idx`. When `ino` is zero the buffer refers to filesystem metadata (i.e.
 * superblock, block group descriptors, block & i-node bitmap, etc.) and `off`
 * offset is given from the start of block device. */
static blk_t *blk_get(uint32_t ino, uint32_t idx) {
  blk_list_t *bucket = &buckets[BUCKET(ino, idx)];
  blk_t *blk = NULL;

  /* Locate a block in the buffer and return it if found. */
#ifndef STUDENT
  TAILQ_FOREACH(blk, bucket, b_hash) {
    if (blk->b_inode == ino && blk->b_index == idx)
      break;
  }

  if (blk != NULL) {
    if (blk->b_refcnt == 0)
      TAILQ_REMOVE(&lrulst, blk, b_link);
    blk->b_refcnt++;
    return blk;
  }
#endif /* STUDENT */

  long blkaddr = ext2_blkaddr_read(ino, idx);
  debug("ext2_blkaddr_read(%d, %d) -> %ld\n", ino, idx, blkaddr);
  if (blkaddr == -1)
    return NULL;
  if (blkaddr == 0)
    return BLK_ZERO;
  if (ino > 0 && !ext2_block_used(blkaddr))
    panic("Attempt to read block %d that is not in use!", blkaddr);

  blk = blk_alloc();
  blk->b_inode = ino;
  blk->b_index = idx;
  blk->b_blkaddr = blkaddr;
  blk->b_refcnt = 1;

  ssize_t nread =
    pread(fd_ext2, blk->b_data, BLKSIZE, blk->b_blkaddr * BLKSIZE);
  if (nread != BLKSIZE)
    panic("Attempt to read past the end of filesystem!");

  TAILQ_INSERT_HEAD(bucket, blk, b_hash);
  return blk;
}

/* Releases a block buffer. If reference counter hits 0 the buffer can be
 * reused to cache another block. The buffer is put at the beginning of LRU list
 * of unused blocks. */
static void blk_put(blk_t *blk) {
  if (--blk->b_refcnt > 0)
    return;

  TAILQ_INSERT_HEAD(&lrulst, blk, b_link);
}

