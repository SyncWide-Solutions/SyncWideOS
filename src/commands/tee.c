#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

void cmd_tee(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    if (*args == '\0') {
        terminal_writestring("Usage: tee <filename>\n");
        terminal_writestring("       (reads from stdin and writes to both stdout and file)\n");
        return;
    }
    
    // Extract filename
    char filename[256];
    size_t i = 0;
    while (*args && *args != ' ' && i < sizeof(filename) - 1) {
        filename[i++] = *args++;
    }
    filename[i] = '\0';
    
    terminal_writestring("tee: Interactive input not yet implemented\n");
    terminal_writestring("Use echo with redirection instead:\n");
    terminal_writestring("  echo \"text\" | tee ");
    terminal_writestring(filename);
    terminal_writestring("\n");
}