#pragma once

/* OverSimplified FileSystem types and functions. */

#include <sys/cdefs.h>
#include <sys/dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

/* Filesystem block size. */
#define FS_BLKSIZE 512

/* The super block offset is given in absolute disk addresses. */
#define FS_SBOFF ((off_t)1024)

/* The root inode is the root of the file system.
 * Inode 0 can't be used for normal purposes  */
#define FS_ROOTINO ((ino_t)1)

/* Filesystem identification. */
#define FS_MAGIC 0x43697269

/* File system super block on the disk. */
typedef struct FsSuperBlock {
  uint32_t magic;      /* magic number */
  uint32_t reserved;   /* reserved for future use */
  uint32_t nblocks;    /* number of all blocks */
  uint32_t nfreeblk;   /* number of free blocks */
  uint32_t ninodes;    /* number of all inodes */
  uint32_t nfreeino;   /* number of free inodes */
  uint32_t inodestart; /* first block with i-nodes */
  uint32_t datastart;  /* first block with user data */
} FsSuperBlock_t;

static_assert(sizeof(FsSuperBlock_t) == 32,
              "size of super-block must be 32 bytes");

/* How many block pointers fit into one block? */
#define FS_NPTRBLK (FS_BLKSIZE / sizeof(uint32_t))

/* How many i-nodes fit into one block? */
#define FS_NINOBLK (FS_BLKSIZE / sizeof(FsInode_t))

#define FS_NDADDR 10 /* Direct addresses in inode. */
#define FS_NIADDR 3  /* Indirect addresses in inode. */
#define FS_NADDR (FS_NDADDR + FS_NIADDR)

/* Structure of an inode on the disk. */
typedef struct FsInode {
  uint16_t mode;             /*   0: IFMT, permissions */
  uint16_t nlink;            /*   2: file link count */
  uint32_t nblock;           /*   4: blocks count */
  uint32_t size;             /*   8: size in bytes */
  uint32_t blocks[FS_NADDR]; /*  12: disk blocks */
} FsInode_t;

static_assert(sizeof(FsInode_t) == 64, "size of i-node must be 64 bytes");

/* File permissions. */
enum {
  FS_IEXEC = 0x01,  /* Executable. */
  FS_IWRITE = 0x02, /* Writable. */
  FS_IREAD = 0x04,  /* Readable. */
};

/* File types. */
enum {
  FS_IFREG = 0x10, /* Regular file. */
  FS_IFDIR = 0x20, /* Directory file. */
  FS_IFCHR = 0x30, /* Character device. */
  FS_IFBLK = 0x40, /* Block device. */
  FS_IFLNK = 0x50, /* Symbolic link. */
  FS_IFMT = 0x70,  /* Mask of file type. */
};

/* Directory file types. */
enum {
  FT_UNKNOWN = 0,
  FT_REG = 1,
  FT_DIR = 2,
  FT_CHRDEV = 3,
  FT_BLKDEV = 4,
  FT_SYMLINK = 5,
};

/* Maximum length of directory entry name. */
#define FS_MAXNAMLEN 27

typedef struct FsDirent {
  uint32_t ino;                /* 0: inode number of entry */
  char name[FS_MAXNAMLEN + 1]; /* 4: name with length <= FS_MAXNAMLEN */
} FsDirent_t;

static_assert(sizeof(FsDirent_t) == 32, "size of FsDirent must be 32 bytes");

typedef struct File File_t;

int FsRead(ino_t ino, IoReq_t *io);
int FsReadDir(ino_t ino, off_t *offp, dirent_t *de);
int FsStat(ino_t ino, struct stat *st);
int FsLookup(ino_t ino, const char *name, ino_t *ino_p, uint8_t *type_p);
int FsMount(File_t *fs);
