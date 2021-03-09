#pragma once

#include <sys/types.h>

#define MAXNAMLEN 14

typedef struct dirent {
  ino_t d_fileno;
  char d_name[MAXNAMLEN];
} dirent_t;
