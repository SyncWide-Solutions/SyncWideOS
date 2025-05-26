#ifndef STRING_H
#define STRING_H

#include <stddef.h>

// Existing string functions
size_t strlen(const char* str);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
char* strcat(char* dest, const char* src);
char* strncat(char* dest, const char* src, size_t n);
char* strchr(const char* s, int c);
char* strrchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);

// Add these declarations for memory functions
void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
int memcmp(const void* s1, const void* s2, size_t n);

// String utility functions
int str_length(const char* str);
int str_compare(const char* str1, const char* str2);
void str_copy(char* dest, const char* src);
void str_copy_n(char* dest, const char* src, int n);
int str_to_int(const char* str);

// Integer to string conversion
char* itoa(int value, char* str, int base);

#endif /* STRING_H */
