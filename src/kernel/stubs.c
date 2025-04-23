#include <stddef.h>

void __mprintf() {}
void* malloc(size_t size) { return NULL; }
void memset(void* ptr, int value, size_t num) {}
void set_int() {}
unsigned char inportb(unsigned short port) { return 0; }
void mutex_lock() {}
void mutex_unlock() {}
void send_eoi() {}
void _kill() {}
