#ifndef _DIRENT_H_
#define _DIRENT_H_

#include <types.h>
#include <floppy.h>
#include <file.h>

/* On disk directory entries are always aligned to 2-byte boundary. */
typedef struct DirEntry {
  uint8_t  reclen;   /* total size of this record in bytes */
  uint8_t  type;     /* type of file (1: executable, 0: regular) */
  uint16_t start;    /* sector where the file begins (0..1759) */
  uint32_t size;     /* file size in bytes (up to 1MiB) */
  char     name[];   /* name of the file (NUL terminated) */
} DirEntry_t;

void FsInit(void);

/* Reads in a directory and allows user to perform FsOpen(...) */
void FsMount(void);

/* Makes further FsOpen(...) to fail ? */
void FsUnMount(void);

/* To start listing directory entries the value of `*base_p` must be NULL.
 * Do not modify `*base_p` as it points to internal filesystem buffer.
 * When FsListDir returns NULL there're no more entries to read! */
const DirEntry_t *FsListDir(void **base_p);

File_t *FsOpen(const char *name);

#endif /* !_DIRENT_H_ */
