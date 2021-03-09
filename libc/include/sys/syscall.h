#pragma once

#define SYS_exit 1
#define SYS_open 2
#define SYS_close 3
#define SYS_read 4
#define SYS_write 5
#define SYS_execv 6
#define SYS_vfork 7
#define SYS_chdir 8
#define SYS_dup 9
#define SYS_fstat 10
#define SYS_kill 11
#define SYS_mkdir 12
#define SYS_pipe 13
#define SYS_stat 14
#define SYS_unlink 15
#define SYS_wait 16

/* Operand classes are described in gcc/config/m68k/m68k.md */
#define SYSCALL0(res, nr)                                                      \
  asm volatile("moveq.l %1,%%d0\n"                                             \
               "trap    #1\n"                                                  \
               "move.l  %%d0,%0\n"                                             \
               : "=rm"(res)                                                    \
               : "i"(nr)                                                       \
               : "memory", "cc", "d0", "d1")

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

#define SYSCALL2(res, nr, arg1, arg2)                                          \
  asm volatile("moveq.l %1,%%d0\n"                                             \
               "move.l  %2,%%d1\n"                                             \
               "move.l  %3,%%d2\n"                                             \
               "trap    #1\n"                                                  \
               "move.l  %%d0,%0\n"                                             \
               : "=rm"(res)                                                    \
               : "i"(nr), "rm"(arg1), "rm"(arg2)                               \
               : "memory", "cc", "d0", "d1", "d2")

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
