#include "../include/string.h"

size_t strlen(const char* str) {
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* original_dest = dest;
    while ((*dest++ = *src++));
    return original_dest;
}

char* strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    
    // Pad with zeros if necessary
    for (; i < n; i++) {
        dest[i] = '\0';
    }
    
    return dest;
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) {
        return 0;
    }
    return (*(unsigned char*)s1 - *(unsigned char*)s2);
}

char* strcat(char* dest, const char* src) {
    char* ptr = dest + strlen(dest);
    while (*src != '\0') {
        *ptr++ = *src++;
    }
    *ptr = '\0';
    return dest;
}

char* strrchr(const char* str, int ch) {
    char* last_occurrence = NULL;
    
    while (*str) {
        if (*str == (char)ch) {
            last_occurrence = (char*)str;
        }
        str++;
    }
    
    return last_occurrence;
}

void* memset(void* s, int c, size_t n) {
    unsigned char* p = (unsigned char*)s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void* memcpy(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (n--) {
        *d++ = *s++;
    }
    return dest;
}

void* memmove(void* dest, const void* src, size_t n) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    
    if (d < s) {
        // Copy from start to end
        while (n--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        // Copy from end to start to handle overlapping regions
        d += n - 1;
        s += n - 1;
        while (n--) {
            *d-- = *s--;
        }
    }
    
    return dest;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    const unsigned char* p1 = (const unsigned char*)s1;
    const unsigned char* p2 = (const unsigned char*)s2;
    
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    
    return 0;
}

// If itoa is not already implemented, add it here
char* itoa(int value, char* str, int base) {
    char* rc;
    char* ptr;
    char* low;
    
    // Check for supported base
    if (base < 2 || base > 36) {
        *str = '\0';
        return str;
    }
    
    rc = ptr = str;
    
    // Set '-' for negative decimals
    if (value < 0 && base == 10) {
        *ptr++ = '-';
    }
    
    // Remember where the numbers start
    low = ptr;
    
    // The actual conversion
    do {
        // Modulo is negative for negative value, this trick makes abs() unnecessary
        *ptr++ = "0123456789abcdefghijklmnopqrstuvwxyz"[((value % base) < 0 ? -(value % base) : (value % base))];
        value /= base;
    } while (value);
    
    // Terminating the string
    *ptr-- = '\0';
    
    // Invert the numbers
    while (low < ptr) {
        char tmp = *low;
        *low++ = *ptr;
        *ptr-- = tmp;
    }
    
    return rc;
}

int atoi(const char* str) {
    int result = 0;
    int sign = 1;
    int i = 0;
    
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }
    
    for (; str[i] != '\0'; i++) {
        if (str[i] >= '0' && str[i] <= '9') {
            result = result * 10 + (str[i] - '0');
        } else {
            break;
        }
    }
    
    return sign * result;
}