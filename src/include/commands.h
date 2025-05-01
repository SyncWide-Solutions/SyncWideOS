#ifndef COMMANDS_H
#define COMMANDS_H

#include <stddef.h>
#include <stdint.h>

// Function prototypes for commands
void terminal_clear(void);
void print_prompt(void);
int strncmp(const char *s1, const char *s2, size_t n);

#endif
