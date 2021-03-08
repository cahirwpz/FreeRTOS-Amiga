#pragma once

#include <sys/types.h>

#define S_IFMT 0170000   /* type of file mask */
#define S_IFDIR 0040000  /* directory */
#define S_IFREG 0100000  /* regular */

typedef struct stat {
  ino_t st_ino;            /* inode's number */
  mode_t st_mode;          /* inode protection mode */
  off_t st_size;           /* file size, in bytes */
} stat_t;

int mkdir(const char *, mode_t);
int stat(const char *pathname, stat_t *sb);
int fstat(int fd, stat_t *sb);
