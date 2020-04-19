#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#include <cdefs.h>
#include <stddef.h>

typedef struct File File_t;

#define SYS_exit 1
#define SYS_open 2
#define SYS_close 3
#define SYS_read 4
#define SYS_write 5
#define SYS_sleep 6

/* Operand classes are described in gcc/config/m68k/m68k.md */
#define SYSCALL1(res, nr, arg1)                                                \
  asm volatile("moveq %1,d0;"                                                  \
               "moveq %2,d1;"                                                  \
               "trap #1;"                                                      \
               "move d0,%0;"                                                   \
               : "=rm"(res)                                                    \
               : "i"(nr), "rm"(arg1)                                           \
               : "memory", "cc", "d0", "d1")

#define SYSCALL1_NR(nr, arg1)                                                  \
  asm volatile("moveq %0,d0;"                                                  \
               "moveq %1,d1;"                                                  \
               "trap #1;"                                                      \
               : /* no output */                                               \
               : "i"(nr), "rm"(arg1)                                           \
               : "memory", "cc", "d0", "d1")

#define SYSCALL3(ret, nr, arg1, arg2, arg3)                                    \
  asm volatile("moveq %1,d0;"                                                  \
               "moveq %2,d1;"                                                  \
               "moveq %3,d2;"                                                  \
               "moveq %4,d3;"                                                  \
               "trap #1;"                                                      \
               "move d0,%0;"                                                   \
               : "=rm"(res)                                                    \
               : "i"(nr), "rm"(arg1), "rm"(arg2), "rm"(arg3)                   \
               : "memory", "cc", "d0", "d1", "d2", "d3")

static inline void exit(int status) {
  SYSCALL1_NR(SYS_exit, status);
}

static inline long read(File_t *fh, void *buf, size_t nbyte) {
  long res;
  SYSCALL3(res, SYS_read, fh, buf, nbyte);
  return res;
}

static inline long write(File_t *fh, const void *buf, size_t nbyte) {
  long res;
  SYSCALL3(res, SYS_write, fh, buf, nbyte);
  return res;
}

static inline File_t *open(const char *path) {
  File_t *fh;
  SYSCALL1(fh, SYS_open, path);
  return fh;
}

static inline void close(File_t *fh) {
  SYSCALL1_NR(SYS_close, fh);
}

static inline void sleep(unsigned miliseconds) {
  SYSCALL1_NR(SYS_sleep, miliseconds);
}

#endif /* !_SYSCALLS_H_ */
