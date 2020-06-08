#ifndef _SYSAPI_H_
#define _SYSAPI_H_

#include <cdefs.h>
#include <stddef.h>

void exit(int status);
long read(int fd, void *buf, size_t nbyte);
long write(int fd, const void *buf, size_t nbyte);
int open(const char *path);
void close(int fd);
void sleep(unsigned miliseconds);

#endif /* !_SYSAPI_H_ */
