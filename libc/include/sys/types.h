#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/cdefs.h>

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef uint16_t ino_t;
typedef uint8_t dev_t;
typedef uint16_t accmode_t; /* access permissions */
typedef uint16_t mode_t;    /* permissions */
typedef int32_t off_t;
typedef int pid_t;
typedef intptr_t ssize_t;

#define major(x) ((x) & 0xf0) >> 4)
#define minor(x) ((x)&0x0f)
