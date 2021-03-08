#pragma once

#define O_RDONLY    0x00000000  /* open for reading only */
#define O_WRONLY    0x00000001  /* open for writing only */
#define O_RDWR      0x00000002  /* open for reading and writing */

#define O_CREAT 0x00000200     /* create if nonexistent */
#define O_TRUNC 0x00000400     /* truncate to zero length */

int open(const char *, int);
