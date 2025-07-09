#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

void* kmalloc(size_t size);
void kfree(void* ptr);
void memory_init(void);

void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);
void* calloc(size_t num, size_t size);

#endif /* MEMORY_H */
