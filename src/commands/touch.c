#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

void cmd_touch(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    if (*args == '\0') {
        terminal_writestring("Usage: touch <filename>\n");
        return;
    }
    
    // Extract filename
    char filename[256];
    size_t i = 0;
    while (*args && *args != ' ' && i < sizeof(filename) - 1) {
        filename[i++] = *args++;
    }
    filename[i] = '\0';
    
    if (fs_is_mounted()) {
        // Check if file already exists
        if (fs_exists(filename)) {
            terminal_writestring("File '");
            terminal_writestring(filename);
            terminal_writestring("' already exists\n");
            return;
        }
        
        // Create empty file
        fs_file_handle_t* file = fs_open(filename, "w");
        if (!file) {
            terminal_writestring("Error: Could not create file '");
            terminal_writestring(filename);
            terminal_writestring("'\n");
            return;
        }
        
        fs_close(file);
        terminal_writestring("Created empty file '");
        terminal_writestring(filename);
        terminal_writestring("'\n");
    } else {
        // Use legacy filesystem
        fs_node_t* parent = fs_get_cwd();
        
        if (fs_find_node(parent, filename)) {
            terminal_writestring("File '");
            terminal_writestring(filename);
            terminal_writestring("' already exists\n");
            return;
        }
        
        fs_node_t* file = fs_create_file(parent, filename);
        if (!file) {
            terminal_writestring("Error: Could not create file '");
            terminal_writestring(filename);
            terminal_writestring("'\n");
            return;
        }
        
        fs_write_file(file, "");
        terminal_writestring("Created empty file '");
        terminal_writestring(filename);
        terminal_writestring("'\n");
    }
}