#ifndef _LIBKERN_H_
#define _LIBKERN_H_

#include <file.h>

extern File_t *KernCons; /* Kernel console file (parallel port by default) */

#define kputchar(c) FilePutChar(KernCons, (c))
#define kprintf(...) FilePrintf(KernCons, __VA_ARGS__)

#define khexdump(ptr, len) FileHexDump(KernCons, (ptr), (len))
#define khexdump_s(ptr) khexdump(ptr, sizeof(*ptr))

void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void kmcheck(int verbose);

__strong_alias(kmalloc, pvPortMalloc);
__strong_alias(kfree, vPortFree);
__strong_alias(kcalloc, pvPortCalloc);
__strong_alias(krealloc, pvPortRealloc);
__strong_alias(kmcheck, vPortMemCheck);

#endif /* !_LIBKERN_H_ */
