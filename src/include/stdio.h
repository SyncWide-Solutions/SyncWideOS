#ifndef STDIO_H
#define STDIO_H

#include <stddef.h>
#include <stdarg.h>

// Minimal stdio for MicroPython
typedef struct {
    int dummy;
} FILE;

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

// Function declarations
int printf(const char *format, ...);
int sprintf(char *str, const char *format, ...);
int snprintf(char *str, size_t size, const char *format, ...);
int vprintf(const char *format, va_list ap);
int vsprintf(char *str, const char *format, va_list ap);
int vsnprintf(char *str, size_t size, const char *format, va_list ap);

int putchar(int c);
int puts(const char *s);

#endif