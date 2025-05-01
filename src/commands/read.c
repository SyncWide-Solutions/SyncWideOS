#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

void cmd_read(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check if a filename was provided
    if (*args == '\0') {
        terminal_writestring("Usage: read <filename>\n");
        return;
    }
    
    // Find the file
    fs_node_t* file = fs_find_node_by_path(args);
    
    if (!file) {
        terminal_writestring("Error: File not found: ");
        terminal_writestring(args);
        terminal_writestring("\n");
        return;
    }
    
    // Check if it's a file (not a directory)
    if (file->type != FS_TYPE_FILE) {
        terminal_writestring("Error: Not a file: ");
        terminal_writestring(args);
        terminal_writestring("\n");
        return;
    }
    
    // Display the file contents
    terminal_writestring("--- File: ");
    terminal_writestring(args);
    terminal_writestring(" ---\n");
    terminal_writestring(file->content);
    terminal_writestring("\n--- End of file ---\n");
}
