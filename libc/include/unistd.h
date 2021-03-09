#pragma once

#include <sys/types.h>

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int chdir(const char *);
int close(int);
int dup(int);
int execv(const char *, char *const *);
int pipe(int fd[2]);
ssize_t read(int, void *, size_t);
void *sbrk(intptr_t);
int unlink(const char *);
pid_t wait(int *);
ssize_t write(int, const void *, size_t);
pid_t vfork(void);
