#include "../include/commands.h"
#include <stddef.h>
#include <stdint.h>

// External function declarations
extern void terminal_writestring(const char* data);

/**
 * Echo command - prints the arguments to the terminal
 * 
 * @param args The command arguments as a string
 */
void cmd_echo(const char* args) {
    // If there are arguments, print them
    if (args && *args) {
        terminal_writestring(args);
    }
    terminal_writestring("\n");
}
