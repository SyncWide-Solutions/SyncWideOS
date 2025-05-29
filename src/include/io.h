#ifndef IO_H
#define IO_H

#include <stdint.h>

// Port I/O functions
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t val) {
    asm volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile ("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outl(uint16_t port, uint32_t val) {
    asm volatile ("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile ("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

// Memory-mapped I/O functions
static inline void mmio_write8(uint32_t addr, uint8_t val) {
    *(volatile uint8_t*)addr = val;
}

static inline uint8_t mmio_read8(uint32_t addr) {
    return *(volatile uint8_t*)addr;
}

static inline void mmio_write16(uint32_t addr, uint16_t val) {
    *(volatile uint16_t*)addr = val;
}

static inline uint16_t mmio_read16(uint32_t addr) {
    return *(volatile uint16_t*)addr;
}

static inline void mmio_write32(uint32_t addr, uint32_t val) {
    *(volatile uint32_t*)addr = val;
}

static inline uint32_t mmio_read32(uint32_t addr) {
    return *(volatile uint32_t*)addr;
}

#endif // IO_H
