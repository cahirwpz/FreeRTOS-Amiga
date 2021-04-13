#pragma once

#include <uae.h>
#include <sys/types.h>

#define klog(...) UaeLog(__VA_ARGS__)

void *kmalloc(size_t size);
void *kmalloc_chip(size_t size);
void kfree(void *ptr);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void kmcheck(int verbose);
