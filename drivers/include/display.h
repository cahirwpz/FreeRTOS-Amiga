#pragma once

#include <sys/ioctl.h>

typedef struct MousePos {
  short x;
  short y;
} MousePos_t;

#define DIOCSETMS _IOW('D', 1, MousePos_t) /* set mouse cursor position */
