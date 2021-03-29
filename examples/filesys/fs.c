#include <device.h>
#include <ioreq.h>
#include <libkern.h>
#include <sys/errno.h>

#define DEBUG 1
#include <debug.h>

#include "fs.h"

/* Properties extracted from a superblock. */
typedef struct Mount {
  FsSuperBlock_t sb;

  /* Calculated from super block. */
  uint32_t inodes_bm;    /* address of inode bitmap */
  uint32_t blocks_bm;    /* address of block bitmap */
  uint32_t inodes_start; /* start of inode table */
  uint32_t blocks_start; /* start of data blocks */

  /* Runtime state */
  Device_t *dev;
  DeviceRead_t readfs;
  DeviceWrite_t writefs;
  IoReq_t io;
} Mount_t;

static Mount_t mnt[1];

/*
 * OverSimplified FileSystem routines.
 */

static void FsReadBlk(uint32_t b_addr, void *rbuf, off_t offset, size_t len) {
  IoReq_t *io = &mnt->io;
  DASSERT(b_addr < mnt->sb.nblocks);
  DASSERT(offset < FS_BLKSIZE);
  io->offset = b_addr * FS_BLKSIZE + offset;
  io->left = len;
  io->rbuf = rbuf;
  io->write = 0;
  mnt->readfs(mnt->dev, io);
  DASSERT(io->left == 0 && io->error == 0);
}

/* Reads block bitmap entry for `b_addr` block address.
 * Returns 0 if the block is free, 1 if it's in use. */
int FsBlockUsed(uint32_t b_addr) {
  DASSERT(b_addr < mnt->sb.nblocks);
  uint32_t b_blk = mnt->blocks_bm + b_addr / (8 * FS_BLKSIZE);
  uint32_t b_off = b_addr % FS_BLKSIZE;
  uint32_t b_bit = b_addr & 7;
  uint8_t b_map;
  FsReadBlk(b_blk, &b_map, b_off, sizeof(uint8_t));
  return (b_map >> b_bit) & 1;
}

/* Reads i-node bitmap entry for `ino`.
 * Returns 0 if the i-node is free, 1 if it's in use. */
int FsInodeUsed(uint32_t ino) {
  DASSERT(ino != 0 && ino < mnt->sb.ninodes);
  --ino;
  /*
   * The first block of this block group is represented by bit 0 of byte 0,
   * the second by bit 1 of byte 0. The 8th block is represented by bit 7
   * (most significant bit) of byte 0 while the 9th block is represented
   * by bit 0 (least significant bit) of byte 1.
   */
  uint32_t i_blk = mnt->inodes_bm + ino / (8 * FS_BLKSIZE);
  uint32_t i_off = ino % FS_BLKSIZE;
  uint32_t i_bit = ino & 7;
  uint8_t i_map;
  FsReadBlk(i_blk, &i_map, i_off, sizeof(uint8_t));
  return (i_map >> i_bit) & 1;
}

/* Reads i-node identified by number `ino`. */
static void FsInodeRead(uint32_t ino, FsInode_t *fi) {
  DASSERT(FsInodeUsed(ino));
  ino--;
  uint32_t fi_pos = ino * sizeof(FsInode_t);
  uint32_t fi_blk = mnt->inodes_start + fi_pos / FS_BLKSIZE;
  uint32_t fi_off = fi_pos % FS_BLKSIZE;
  FsReadBlk(fi_blk, fi, fi_off, sizeof(FsInode_t));
}

/* Returns block pointer `p_idx` from block of `p_blk` address. */
static uint32_t FsBlkPtrRead(uint32_t p_blk, uint32_t p_idx) {
  DASSERT(p_idx < FS_NPTRBLK);
  uint32_t blkptr;
  FsReadBlk(p_blk, &blkptr, p_idx * sizeof(uint32_t), sizeof(uint32_t));
  return blkptr;
}

/* Translates i-node number `ino` and block index `idx` to block address.
 * Returns -1 on failure, otherwise block address. */
int32_t FsBlkAddrRead(uint32_t ino, uint32_t blkidx) {
  /* No translation for filesystem metadata blocks. */
  if (ino == 0)
    return blkidx;

  FsInode_t inode;
  FsInodeRead(ino, &inode);

  /* Read direct pointers or pointers from indirect blocks. */
  /* Check if `blkidx` is not past the end of file. */
  if (blkidx * FS_BLKSIZE >= inode.size)
    return -1;

  if (blkidx < FS_NDADDR)
    return inode.blocks[blkidx];

  blkidx -= FS_NDADDR;
  if (blkidx < FS_NPTRBLK)
    return FsBlkPtrRead(inode.blocks[FS_NDADDR], blkidx);

  uint32_t l2blkaddr, l3blkaddr, l1blkidx;

  blkidx -= FS_NPTRBLK;
  if (blkidx < FS_NPTRBLK * FS_NPTRBLK) {
    l2blkaddr = FsBlkPtrRead(inode.blocks[FS_NDADDR + 1], blkidx / FS_NPTRBLK);
    return FsBlkPtrRead(l2blkaddr, blkidx % FS_NPTRBLK);
  }

  blkidx -= FS_NPTRBLK * FS_NPTRBLK;
  l1blkidx = blkidx / (FS_NPTRBLK * FS_NPTRBLK);
  blkidx %= FS_NPTRBLK * FS_NPTRBLK;
  l2blkaddr = FsBlkPtrRead(inode.blocks[FS_NDADDR + 2], l1blkidx);
  l3blkaddr = FsBlkPtrRead(l2blkaddr, blkidx / FS_NPTRBLK);
  return FsBlkPtrRead(l3blkaddr, blkidx % FS_NPTRBLK);
}

#if 0
/* Reads exactly `len` bytes starting from `pos` position from any file (i.e.
 * regular, directory, etc.) identified by `ino` i-node. Returns 0 on success,
 * EINVAL if `pos` and `len` would have pointed past the last block of file.
 *
 * WARNING: This function assumes that `ino` i-node pointer is valid! */
int FsRead(ino_t ino, IoReq_t *io) {
  DPRINTF("FsRead(%d, %p, %ld, %ld)\n", ino, io->rbuf, io->offset, io->left);

  long off = pos % FS_BLKSIZE;
  long idx = pos / FS_BLKSIZE;

  while (io->left > 0) {
    size_t cnt = min(BLKSIZE - off, len);
    blk_t *blk = blk_get(ino, idx++);
    if (blk == NULL)
      return EINVAL;
    if (blk == BLK_ZERO) {
      memset(data, 0, cnt);
    } else {
      memcpy(data, blk->b_data + off, cnt);
      blk_put(blk);
    }
    data += cnt;
    len -= cnt;
    off = 0;
  }
  return 0;
}

/* Reads a directory entry at position stored in `off_p` from `ino` i-node that
 * is assumed to be a directory file. The entry is stored in `de` and
 * `de->de_name` must be NUL-terminated. Assumes that entry offset is 0 or was
 * set by previous call to `ext2_readdir`. Returns 1 on success, 0 if there are
 * no more entries to read. */
#define de_name_offset offsetof(ext2_dirent_t, de_name)

int ext2_readdir(uint32_t ino, uint32_t *off_p, ext2_dirent_t *de) {
#ifndef STUDENT
  de->de_ino = 0;
  while (true) {
    if (ext2_read(ino, de, *off_p, de_name_offset))
      return 0;
    if (de->de_ino)
      break;
    /* Bogus entry -> skip it! */
    *off_p += de->de_reclen;
  }
  if (ext2_read(ino, &de->de_name, *off_p + de_name_offset, de->de_namelen))
    panic("Attempt to read past the end of directory!");
  de->de_name[de->de_namelen] = '\0';
  *off_p += de->de_reclen;
  return 1;
#endif /* STUDENT */
  return 0;
}

/* Read the target of a symbolic link identified by `ino` i-node into buffer
 * `buf` of size `buflen`. Returns 0 on success, EINVAL if the file is not a
 * symlink or read failed. */
int ext2_readlink(uint32_t ino, char *buf, size_t buflen) {
  int error;

  ext2_inode_t inode;
  if ((error = ext2_inode_read(ino, &inode)))
    return error;

    /* Check if it's a symlink and read it. */
#ifndef STUDENT
  if (!(inode.i_mode & FS_IFLNK))
    return EINVAL;

  size_t len = min(inode.i_size, buflen);
  if (inode.i_size < FS_MAXSYMLINKLEN) {
    memcpy(buf, inode.i_blocks, len);
  } else {
    if ((error = ext2_read(ino, buf, 0, len)))
      return error;
  }

  return 0;
#endif /* STUDENT */
  return ENOTSUP;
}

/* Read metadata from file identified by `ino` i-node and convert it to
 * `struct stat`. Returns 0 on success, or error if i-node could not be read. */
int ext2_stat(uint32_t ino, struct stat *st) {
  int error;

  ext2_inode_t inode;
  if ((error = ext2_inode_read(ino, &inode)))
    return error;

    /* Convert the metadata! */
#ifndef STUDENT
  st->st_dev = 0; /* ??? */
  st->st_mode = inode.i_mode;
  st->st_ino = ino;
  st->st_nlink = inode.i_nlink;
  st->st_uid = inode.i_uid;
  st->st_gid = inode.i_gid;
  st->st_rdev = 0; /* ??? */
  st->st_atime = inode.i_atime;
  st->st_mtime = inode.i_mtime;
  st->st_ctime = inode.i_ctime;
  st->st_size = inode.i_size;
  st->st_blocks = inode.i_nblock;
  st->st_blksize = BLKSIZE;
#ifdef __APPLE__
  st->st_flags = inode.i_flags;
  st->st_gen = inode.i_gen;
#endif
  return 0;
#endif /* STUDENT */
  return ENOTSUP;
}

/* Reads file identified by `ino` i-node as directory and performs a lookup of
 * `name` entry. If an entry is found, its i-inode number is stored in `ino_p`
 * and its type in stored in `type_p`. On success returns 0, or EINVAL if `name`
 * is NULL or zero length, or ENOTDIR is `ino` file is not a directory, or
 * ENOENT if no entry was found. */
int ext2_lookup(uint32_t ino, const char *name, uint32_t *ino_p,
                uint8_t *type_p) {
  int error;

  if (name == NULL)
    return EINVAL;

  ext2_inode_t inode;
  if ((error = ext2_inode_read(ino, &inode)))
    return error;

#ifndef STUDENT
  if (!(inode.i_mode & FS_IFDIR))
    return ENOTDIR;

  if (!strlen(name))
    return EINVAL;

  ext2_dirent_t de;
  uint32_t off = 0;
  while (ext2_readdir(ino, &off, &de)) {
    if (!strcmp(name, de.de_name)) {
      if (ino_p)
        *ino_p = de.de_ino;
      if (type_p)
        *type_p = de.de_type;
      return 0;
    }
  }
#endif /* STUDENT */
  return ENOENT;
}

/* Initializes ext2 filesystem stored in `fspath` file.
 * Returns 0 on success, otherwise an error. */
int ext2_mount(const char *fspath) {
  int error;

  if ((error = blk_init(fspath)))
    return error;

  /* Read superblock and verify we support filesystem's features. */
  ext2_superblock_t sb;
  ext2_read(0, &sb, FS_SBOFF, sizeof(ext2_superblock_t));

  debug(">>> super block\n"
        "# of inodes      : %d\n"
        "# of blocks      : %d\n"
        "block size       : %ld\n"
        "blocks per group : %d\n"
        "inodes per group : %d\n"
        "inode size       : %d\n",
        sb.sb_icount, sb.sb_bcount, 1024UL << sb.sb_log_bsize, sb.sb_bpg,
        sb.sb_ipg, sb.sb_inode_size);

  if (sb.sb_magic != FS_MAGIC)
    panic("'%s' cannot be identified as ext2 filesystem!", fspath);

  if (sb.sb_rev != FS_REV1)
    panic("Only ext2 revision 1 is supported!");

  size_t blksize = 1024UL << sb.sb_log_bsize;
  if (blksize != BLKSIZE)
    panic("ext2 filesystem with block size %ld not supported!", blksize);

  if (sb.sb_inode_size != sizeof(ext2_inode_t))
    panic("The only i-node size supported is %d!", sizeof(ext2_inode_t));

    /* Load interesting data from superblock into global variables.
     * Read group descriptor table into memory. */
#ifndef STUDENT
  block_count = sb.sb_bcount;
  inode_count = sb.sb_icount;
  group_desc_count = howmany(sb.sb_bcount, sb.sb_bpg);
  blocks_per_group = sb.sb_bpg;
  inodes_per_group = sb.sb_ipg;
  first_data_block = sb.sb_first_dblock;

  size_t gd_size = group_desc_count * sizeof(ext2_groupdesc_t);
  group_desc = malloc(gd_size);
  ext2_read(0, group_desc, FS_GDOFF, gd_size);

  debug("\n>>> block group descriptor table (%ld entries)\n\n",
        group_desc_count);
  for (size_t i = 0; i < group_desc_count; i++) {
    ext2_groupdesc_t *gd __unused = &group_desc[i];
    debug("* block group descriptor %ld (has backup: %s):\n"
          "  i-nodes: %ld - %ld, blocks: %d - %ld, directories: %d\n"
          "  blocks bitmap: %d, inodes bitmap: %d, inodes table: %d\n",
          i, ext2_gd_has_backup(i) ? "yes" : "no", 1 + i * inodes_per_group,
          (i + 1) * inodes_per_group, gd->gd_b_bitmap,
          min(gd->gd_b_bitmap + blocks_per_group, block_count) - 1,
          gd->gd_ndirs, gd->gd_b_bitmap, gd->gd_i_bitmap, gd->gd_i_tables);
  }
  return 0;
#endif /* STUDENT */
  return ENOTSUP;
}
#endif
