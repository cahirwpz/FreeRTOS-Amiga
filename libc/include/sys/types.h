#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef uint32_t daddr_t;  /* disk address */
typedef uint32_t ino_t;    /* i-node number */
typedef uint32_t blkcnt_t; /* fs block count */
typedef uint8_t dev_t;
typedef uint8_t mode_t;
typedef int32_t off_t;
typedef int pid_t;
typedef intptr_t ssize_t;

#define major(x) ((x) & 0xf0) >> 4)
#define minor(x) ((x)&0x0f)
