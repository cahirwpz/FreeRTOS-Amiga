#ifndef _SYSAPI_H_
#define _SYSAPI_H_

#include <cdefs.h>
#include <stddef.h>

#include "syscall.h"

typedef struct File File_t;

void exit(int status);
long read(File_t *fh, void *buf, size_t nbyte);
long write(File_t *fh, const void *buf, size_t nbyte);
File_t *open(const char *path);
void close(File_t *fh);
void sleep(unsigned miliseconds);

#endif /* !_SYSAPI_H_ */
