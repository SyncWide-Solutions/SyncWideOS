#ifndef FAT32_H
#define FAT32_H

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

// Function declarations
bool fat32_init(void);
bool fat32_mount(void);

#endif /* FAT32_H */