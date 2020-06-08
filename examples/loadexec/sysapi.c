#include "sysapi.h"

#define SYS_exit 1
#define SYS_open 2
#define SYS_close 3
#define SYS_read 4
#define SYS_write 5
#define SYS_sleep 6

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

#define SYSCALL3(ret, nr, arg1, arg2, arg3)                                    \
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

int open(const char *path) {
  int fd;
  SYSCALL1(fd, SYS_open, path);
  return fd;
}

void close(int fd) {
  SYSCALL1_NR(SYS_close, fd);
}

void sleep(unsigned miliseconds) {
  SYSCALL1_NR(SYS_sleep, miliseconds);
}
