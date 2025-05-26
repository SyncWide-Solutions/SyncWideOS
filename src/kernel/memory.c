#include <stddef.h>
#include <stdint.h>
#include "../include/memory.h"

// Simple memory allocator using a static buffer
#define HEAP_SIZE (1024 * 1024)  // 1MB heap
static uint8_t heap[HEAP_SIZE];
static size_t heap_offset = 0;

void* kmalloc(size_t size) {
    // Align to 4-byte boundary
    size = (size + 3) & ~3;
    
    if (heap_offset + size > HEAP_SIZE) {
        return NULL; // Out of memory
    }
    
    void* ptr = &heap[heap_offset];
    heap_offset += size;
    return ptr;
}

void kfree(void* ptr) {
    // Simple allocator - we don't actually free memory
    // In a real OS, you'd implement proper memory management
    (void)ptr;
}

void memory_init(void) {
    heap_offset = 0;
}