#include <file.h>

static const char hex2char[16] = "0123456789abcdef";

void FileHexDump(File_t *f, void *ptr, size_t length) {
  unsigned char *data = ptr;
  char buf[5];

  for (size_t i = 0; i < length; i++) {
    if ((i & 15) == 0)
      FilePrintf(f, "%08x:", (intptr_t)data);
    /* optimize the common case */
    unsigned byte = *data++;
    unsigned len = 3;
    buf[0] = ' ';
    buf[1] = hex2char[byte >> 4];
    buf[2] = hex2char[byte & 15];
    if ((i & 3) == 3)
      buf[len++] = ' ';
    if ((i & 15) == 15)
      buf[len++] = '\n';
    FileWrite(f, buf, len);
  }
  FilePutChar(f, '\n');
}
