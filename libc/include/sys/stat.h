#pragma once

#include <sys/types.h>

#define S_IFMT 070  /* type of file mask */
#define S_IFCHR 010 /* character special */
#define S_IFDIR 020 /* directory */
#define S_IFBLK 030 /* block special */
#define S_IFREG 040 /* regular */

#define S_IREAD 004
#define S_IWRITE 002
#define S_IEXEC 001

typedef struct stat {
  dev_t st_dev;   /* inode's device */
  ino_t st_ino;   /* inode's number */
  dev_t st_rdev;  /* device type */
  mode_t st_mode; /* inode type */
  off_t st_size;  /* file size, in bytes */
} stat_t;

int mkdir(const char *);
int stat(const char *pathname, stat_t *sb);
int fstat(int fd, stat_t *sb);
