#pragma once

#include <uae.h>
#include <sys/fcntl.h>
#include <sys/types.h>

typedef struct File File_t;

File_t *kopen(const char *name, int oflag);
long kfread(File_t *f, void *buf, size_t len);
long kfwrite(File_t *f, const void *buf, size_t len);
long kfseek(File_t *f, long offset, int whence);
void kfputchar(File_t *f, char c);
void kfprintf(File_t *f, const char *fmt, ...);
void kfhexdump(File_t *f, void *ptr, size_t length);

#define klog(...) UaeLog(__VA_ARGS__)

void *kmalloc(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void kmcheck(int verbose);
