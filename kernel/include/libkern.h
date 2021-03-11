#pragma once

#include <file.h>
#include <sys/fcntl.h>

extern File_t *KernCons; /* Kernel console file (parallel port by default) */

long kfread(File_t *f, void *buf, size_t len);
long kfwrite(File_t *f, const void *buf, size_t len);
long kfseek(File_t *f, long offset, int whence);
void kfputchar(File_t *f, char c);
void kfprintf(File_t *f, const char *fmt, ...);
void kfhexdump(File_t *f, void *ptr, size_t length);

#define kputchar(c) kfputchar(KernCons, (c))
#define kprintf(...) kfprintf(KernCons, __VA_ARGS__)
#define khexdump(ptr, len) kfhexdump(KernCons, (ptr), (len))
#define khexdump_s(ptr) kfhexdump(KernCons, (ptr), sizeof(*(ptr)))

void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void kmcheck(int verbose);
