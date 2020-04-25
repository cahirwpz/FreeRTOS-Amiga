#ifndef _DIRENT_H_
#define _DIRENT_H_

#include <stdint.h>

/* On disk directory entries are always aligned to 2-byte boundary. */
typedef struct DirEntry {
  uint8_t  reclen;   /* total size of this record in bytes */
  uint8_t  type;     /* type of file (1: executable, 0: regular) */
  uint16_t start;    /* sector where the file begins (0..1759) */
  uint32_t size;     /* file size in bytes (up to 1MiB) */
  char     name[];   /* name of the file (NUL terminated) */
} DirEntry_t

#endif /* !_DIRENT_H_ */
