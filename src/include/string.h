#ifndef STRING_H
#define STRING_H

#include <stddef.h>

// String functions
size_t strlen(const char* str);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
int strcmp(const char* str1, const char* str2);
int strncmp(const char* str1, const char* str2, size_t n);
char* strcat(char* dest, const char* src);
char* strchr(const char* str, int c);
void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t n);
int memcmp(const void* ptr1, const void* ptr2, size_t num);

// Conversion functions
int atoi(const char* str);
char* itoa(int value, char* str, int base);

#endif /* STRING_H */
