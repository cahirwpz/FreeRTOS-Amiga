#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>

/* Operand classes are described in gcc/config/m68k/m68k.md */
#define SYSCALL1(res, nr, arg1)                                                \
  asm volatile("moveq.l %1,%%d0\n"                                             \
               "move.l  %2,%%d1\n"                                             \
               "trap    #1\n"                                                  \
               "move.l  %%d0,%0\n"                                             \
               : "=rm"(res)                                                    \
               : "i"(nr), "rm"(arg1)                                           \
               : "memory", "cc", "d0", "d1")

#define SYSCALL1_NR(nr, arg1)                                                  \
  asm volatile("moveq.l %0,%%d0\n"                                             \
               "move.l  %1,%%d1\n"                                             \
               "trap    #1\n"                                                  \
               : /* no output */                                               \
               : "i"(nr), "rm"(arg1)                                           \
               : "memory", "cc", "d0", "d1")

#define SYSCALL3(res, nr, arg1, arg2, arg3)                                    \
  asm volatile("moveq.l %1,%%d0\n"                                             \
               "move.l  %2,%%d1\n"                                             \
               "move.l  %3,%%d2\n"                                             \
               "move.l  %4,%%d3\n"                                             \
               "trap    #1\n"                                                  \
               "move.l  %%d0,%0\n"                                             \
               : "=rm"(res)                                                    \
               : "i"(nr), "rm"(arg1), "rm"(arg2), "rm"(arg3)                   \
               : "memory", "cc", "d0", "d1", "d2", "d3")

void exit(int status) {
  SYSCALL1_NR(SYS_exit, status);
  for(;;);
}

long read(int fd, void *buf, size_t nbyte) {
  long res;
  SYSCALL3(res, SYS_read, fd, buf, nbyte);
  return res;
}

long write(int fd, const void *buf, size_t nbyte) {
  long res;
  SYSCALL3(res, SYS_write, fd, buf, nbyte);
  return res;
}

int open(const char *path, int mode) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  (void)mode;
  return -1;
}

int close(int fd) {
  int err;
  SYSCALL1(err, SYS_close, fd);
  return err;
}

int vfork(void) {
  /* TODO: Implement system call wrapper. */
  return -1;
}

int execv(const char *path, char *const argv[]) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  (void)argv;
  return -1;
}

int wait(int *statusp) {
  /* TODO: Implement system call wrapper. */
  (void)statusp;
  return -1;
}

int dup(int fd) {
  /* TODO: Implement system call wrapper. */
  (void)fd;
  return -1;
}

int kill(pid_t pid, int sig) {
  /* TODO: Implement system call wrapper. */
  (void)pid;
  (void)sig;
  return -1;
}

int mkdir(const char *path, mode_t mode) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  (void)mode;
  return -1;
}


int fstat(int fd, struct stat *sb) {
  /* TODO: Implement system call wrapper. */
  (void)fd;
  (void)sb;
  return -1;
}

int stat(const char *path, struct stat *sb) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  (void)sb;
  return -1;
}

int unlink(const char *path) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  return -1;
}

int pipe(int fd[2]) {
  /* TODO: Implement system call wrapper. */
  (void)fd;
  return -1;
}

void *sbrk(intptr_t incr) {
  /* TODO: Implement system call wrapper. */
  (void)incr;
  return NULL;
}

int chdir(const char *path) {
  /* TODO: Implement system call wrapper. */
  (void)path;
  return -1;
}
