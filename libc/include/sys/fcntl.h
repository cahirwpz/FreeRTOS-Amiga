#pragma once

#define SEEK_SET 0 /* set file offset to offset */
#define SEEK_CUR 1 /* set file offset to current plus offset */
#define SEEK_END 2 /* set file offset to EOF plus offset */

#define O_RDONLY 0x0000 /* open for reading only */
#define O_WRONLY 0x0001 /* open for writing only */
#define O_RDWR 0x0002   /* open for reading and writing */

#define O_CREAT 0x0200 /* create if nonexistent */
#define O_TRUNC 0x0400 /* truncate to zero length */

int open(const char *, int);
