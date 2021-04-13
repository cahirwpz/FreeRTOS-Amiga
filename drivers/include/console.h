#pragma once

#include <sys/ioctl.h>

typedef struct MousePos {
  short x;
  short y;
} MousePos_t;

#define CIOCSETMS _IOW('c', 1, MousePos_t) /* set mouse cursor position */
