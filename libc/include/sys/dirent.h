#pragma once

#include <sys/types.h>

#define MAXNAMLEN 255

typedef struct dirent {
  ino_t d_fileno;    /* file number of entry */
  uint16_t d_reclen; /* length of this record */
  uint16_t d_namlen; /* length of string in d_name */
  char d_name[MAXNAMLEN];
} dirent_t;
