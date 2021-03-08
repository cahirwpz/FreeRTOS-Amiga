#ifndef _SYSAPI_H_
#define _SYSAPI_H_

#include <sys/cdefs.h>
#include <stddef.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1

void exit(int status);
long read(int fd, void *buf, size_t nbyte);
long write(int fd, const void *buf, size_t nbyte);
int open(const char *path);
void close(int fd);
int vfork(void);
int execv(const char *path, char *const argv[]);
int wait(int *statusp);
void sleep(unsigned miliseconds);

#endif /* !_SYSAPI_H_ */
