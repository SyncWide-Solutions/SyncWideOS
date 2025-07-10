#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

void cmd_rm(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    if (*args == '\0') {
        terminal_writestring("Usage: rm <filename>\n");
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
        // Check if file exists
        if (!fs_exists(filename)) {
            terminal_writestring("Error: File '");
            terminal_writestring(filename);
            terminal_writestring("' not found\n");
            return;
        }
        
        // Delete the file
        if (fs_delete(filename)) {
            terminal_writestring("File '");
            terminal_writestring(filename);
            terminal_writestring("' deleted successfully\n");
        } else {
            terminal_writestring("Error: Could not delete file '");
            terminal_writestring(filename);
            terminal_writestring("'\n");
        }
    } else {
        // Use legacy filesystem
        fs_node_t* file = fs_find_node_by_path(filename);
        if (!file) {
            terminal_writestring("Error: File '");
            terminal_writestring(filename);
            terminal_writestring("' not found\n");
            return;
        }
        
        if (file->type != FS_TYPE_FILE) {
            terminal_writestring("Error: '");
            terminal_writestring(filename);
            terminal_writestring("' is not a file (use rmdir for directories)\n");
            return;
        }
        
        // Remove from parent's children list
        fs_node_t* parent = file->parent;
        if (parent) {
            for (size_t i = 0; i < parent->child_count; i++) {
                if (parent->children[i] == file) {
                    // Shift remaining children
                    for (size_t j = i; j < parent->child_count - 1; j++) {
                        parent->children[j] = parent->children[j + 1];
                    }
                    parent->child_count--;
                    break;
                }
            }
        }
        
        terminal_writestring("File '");
        terminal_writestring(filename);
        terminal_writestring("' deleted successfully\n");
    }
}