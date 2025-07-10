#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

void cmd_mkdir(const char* args) {
    if (!fs_is_mounted()) {
        terminal_writestring("mkdir: FAT32 filesystem not mounted\n");
        return;
    }
    
    if (!args || !*args) {
        terminal_writestring("mkdir: missing operand\n");
        return;
    }
    
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check for empty argument after spaces
    if (!*args) {
        terminal_writestring("mkdir: missing operand\n");
        return;
    }
    
    // Check if directory already exists
    if (fs_exists(args)) {
        terminal_writestring("mkdir: cannot create directory '");
        terminal_writestring(args);
        terminal_writestring("': File exists\n");
        return;
    }
    
    // Create the directory
    if (fs_mkdir(args)) {
        terminal_writestring("Directory created: ");
        terminal_writestring(args);
        terminal_writestring("\n");
    } else {
        terminal_writestring("mkdir: cannot create directory '");
        terminal_writestring(args);
        terminal_writestring("': Operation not supported (FAT32 write operations not implemented)\n");
    }
}
