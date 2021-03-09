#include <sys/syscall.h>
#include <unistd.h>

void *sbrk(intptr_t incr) {
  /* TODO: Implement system call wrapper. */
  (void)incr;
  return NULL;
}
