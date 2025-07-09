#ifndef STDLIB_H
#define STDLIB_H

#include <stddef.h>

// Memory allocation
void* malloc(size_t size);
void free(void* ptr);
void* realloc(void* ptr, size_t size);
void* calloc(size_t num, size_t size);

// String conversion
int atoi(const char *str);
long atol(const char *str);
double atof(const char *str);

// Other utilities
void abort(void);
void exit(int status);

#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1

#endif