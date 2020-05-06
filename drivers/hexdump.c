#include <file.h>
#include <custom.h>

static const char hex2char[16] = "0123456789abcdef";

#pragma GCC push_options
#pragma GCC optimize ("-O2")

void FileHexDump(File_t *f, void *ptr, size_t length) {
  unsigned char *data = ptr;
  char buf[80];
  char *s = buf;

  for (size_t i = 0; i < length; i++) {
    if ((i & 15) == 0) {
      uintptr_t p = (uintptr_t)data;
      for (int k = 7; k >= 0; k--) {
        s[k] = hex2char[p & 15];
        p >>= 4;
      }
      s += 8;
      *s++ = ':';
    }
    int byte = *data++;
    *s++ = ' ';
    *s++ = hex2char[byte >> 4];
    *s++ = hex2char[byte & 15];
    if ((i & 3) == 3)
      *s++ = ' ';
    if ((i & 15) == 15) {
      *s++ = '\n';
      FileWrite(f, buf, s - buf);
      s = buf;
    }
  }
  if (s > buf) {
    *s++ = '\n';
    FileWrite(f, buf, s - buf);
  }
}

#pragma GCC pop_options
