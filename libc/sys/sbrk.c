#include <sys/cdefs.h>
#include <stdlib.h>
#include <unistd.h>

#define MEMSIZE (32 * 1024)

static uint8_t _memory[MEMSIZE] __aligned(8);
static intptr_t _memend = (intptr_t)_memory;

void *sbrk(intptr_t incr) {
  void *ptr = (void *)_memend;
  _memend = roundup(_memend + incr, sizeof(uint64_t));
  if (_memend > (intptr_t)(_memory + MEMSIZE))
    exit(1);
  return ptr;
}
