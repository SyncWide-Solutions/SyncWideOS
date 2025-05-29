#include "../include/io.h"

// String input from port (16-bit words)
void insw(uint16_t port, uint16_t* buffer, uint32_t count) {
    asm volatile("cld; rep insw" : "+D" (buffer), "+c" (count) : "d" (port) : "memory");
}

// String output to port (16-bit words)
void outsw(uint16_t port, uint16_t* buffer, uint32_t count) {
    asm volatile("cld; rep outsw" : "+S" (buffer), "+c" (count) : "d" (port) : "memory");
}
