#include <file.h>

void FileHexDump(File_t *f, void *ptr, size_t length) {
  unsigned char *data = ptr;
  for (size_t i = 0; i < length; i++) {
    if ((i & 15) == 0)
      FilePrintf(f, "%08x:", (intptr_t)data);
    unsigned char byte = *data++;
    FilePrintf(f, " %02x", (int)byte);
    if ((i & 3) == 3)
      FilePutChar(f, ' ');
    if ((i & 15) == 15)
      FilePutChar(f, '\n');
  }
  FilePutChar(f, '\n');
}
