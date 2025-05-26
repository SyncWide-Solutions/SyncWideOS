#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

// Simple current directory tracking
static char current_directory[256] = "/";

void cmd_cd(const char* args) {
    if (!fs_is_mounted()) {
        terminal_writestring("cd: filesystem not mounted\n");
        return;
    }
    
    const char* target_path = "/"; // Default to root
    
    if (args && *args) {
        // Skip leading spaces
        while (*args == ' ') args++;
        if (*args) {
            target_path = args;
        }
    }
    
    // Check if the path exists
    if (!fs_exists(target_path)) {
        terminal_writestring("cd: cannot access '");
        terminal_writestring(target_path);
        terminal_writestring("': No such directory\n");
        return;
    }
    
    // Update current directory
    strncpy(current_directory, target_path, sizeof(current_directory) - 1);
    current_directory[sizeof(current_directory) - 1] = '\0';
}

// Helper function to get current directory
const char* fs_get_current_directory(void) {
    return current_directory;
}
