#define __MODULE_NAME "STUBS"

#include <stddef.h>
#include <stdint.h>
#include <stdarg.h>
#include "../include/string.h"
#include "../include/display.h"
#include "../include/hal.h"

// Declare terminal functions from kernel.c
extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern size_t terminal_row;
extern size_t terminal_column;
extern void update_cursor(int row, int col);

int debug_enabled = 0;  // Debug flag

void __mprintf(char *m, ...) {
    if (!debug_enabled) return;
    va_list args;
    va_start(args, m);
    
    // Prefix with module name
    terminal_writestring("[");
    terminal_writestring(__MODULE_NAME);
    terminal_writestring("] ");
    update_cursor(terminal_row, terminal_column);  // Update cursor after module name
    
    // Basic string formatting (simplified)
    while (*m) {
        if (*m == '%') {
            m++;
            switch (*m) {
                case 's': {
                    char* str = va_arg(args, char*);
                    terminal_writestring(str);
                    update_cursor(terminal_row, terminal_column);  // Update cursor after each string
                    break;
                }
                default:
                    terminal_putchar(*m);
                    update_cursor(terminal_row, terminal_column);  // Update cursor after each character
            }
        } else {
            terminal_putchar(*m);
            update_cursor(terminal_row, terminal_column);  // Update cursor after each character
        }
        m++;
    }
    
    va_end(args);
}

// Global variables for memory management
uint32_t last_alloc = 0x100000;  // Start of heap
uint32_t heap_begin = 0x100000;
uint32_t heap_end = 0x400000;    // 4MB limit
uint32_t memory_used = 0;

// Port I/O functions
unsigned char inportb(unsigned short port) {
    unsigned char result;
    asm volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
    return result;
}

void outportb(unsigned short port, unsigned char value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Memory management functions
char* malloc(size_t size) {
    if(!size) return 0;
    
    // If heap hasn't been initialized, use a default location
    if (heap_begin == 0) {
        heap_begin = 0x100000;  // 1MB mark
        heap_end = 0x400000;    // 4MB mark
        last_alloc = heap_begin;
    }
    
    // Simple linear allocation
    char* mem = (char*)last_alloc;
    last_alloc += size;
    
    // Check if we've exceeded heap
    if (last_alloc >= heap_end) {
        return 0;  // Out of memory
    }
    
    return mem;
}

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = ptr;
    while(num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

// Interrupt and task management stubs
void set_int(int i, uint32_t addr) {
    // Stub for interrupt setup
    //terminal_writestring("[STUBS] Setting interrupt ");
    
    // Convert interrupt number to string
    char buf[10];
    int j = 0, is_negative = 0;
    int val = i;
    
    if (val < 0) {
        is_negative = 1;
        val = -val;
    }
    
    do {
        buf[j++] = val % 10 + '0';
        val /= 10;
    } while (val > 0);
    
    if (is_negative) {
        buf[j++] = '-';
    }
    
    // Reverse the string
    for (int k = 0; k < j/2; k++) {
        char temp = buf[k];
        buf[k] = buf[j-1-k];
        buf[j-1-k] = temp;
    }
    buf[j] = '\0';
    
    //terminal_writestring(buf);
    //terminal_writestring(" at address 0x");
    
    // Convert address to hex
    char hex[10];
    int k = 0;
    val = addr;
    do {
        int digit = val % 16;
        hex[k++] = (digit < 10) ? 
            (digit + '0') : 
            (digit - 10 + 'a');
        val /= 16;
    } while (val > 0);
    
    // Reverse the hex string
    for (int m = 0; m < k/2; m++) {
        char temp = hex[m];
        hex[m] = hex[k-1-m];
        hex[k-1-m] = temp;
    }
    hex[k] = '\0';
    
    //terminal_writestring(hex);
    //terminal_writestring("\n");
    
    // Update cursor after the entire message is printed
    update_cursor(terminal_row, terminal_column);
}

void send_eoi(uint8_t irq) {
    // Basic EOI stub
}

void set_task(int task) {
    // Basic task management stub
}

// Mutex stubs
void mutex_lock(void* mutex) {}
void mutex_unlock(void* mutex) {}

// Process termination
void _kill() {
    while(1) {
        asm volatile ("cli");
        asm volatile ("hlt");
    }
}
