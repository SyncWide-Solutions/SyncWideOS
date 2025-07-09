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

void* malloc(size_t size) {
    // Simple malloc implementation - you may want to improve this
    static char heap_space[1024 * 1024]; // 1MB heap
    static size_t heap_offset = 0;
    
    if (heap_offset + size < sizeof(heap_space)) {
        void* ptr = &heap_space[heap_offset];
        heap_offset += size;
        return ptr;
    }
    return NULL; // Out of memory
}

void free(void* ptr) {
    // Simple implementation - doesn't actually free memory
    // You'd want a proper allocator for production use
    (void)ptr;
}

void* realloc(void* ptr, size_t size) {
    // Simple implementation
    if (ptr == NULL) {
        return malloc(size);
    }
    // For now, just return the same pointer
    // A real implementation would copy data to new location
    return ptr;
}

void* calloc(size_t num, size_t size) {
    size_t total = num * size;
    void* ptr = malloc(total);
    if (ptr) {
        // Zero the memory
        char* p = (char*)ptr;
        for (size_t i = 0; i < total; i++) {
            p[i] = 0;
        }
    }
    return ptr;
}

void memory_init(void) {
    heap_offset = 0;
}