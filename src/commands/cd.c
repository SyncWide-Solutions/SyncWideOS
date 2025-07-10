#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

void cmd_cd(const char* args) {
    if (!fs_is_mounted()) {
        terminal_writestring("cd: FAT32 filesystem not mounted\n");
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
    
    // Try to change directory
    if (!fs_chdir(target_path)) {
        terminal_writestring("cd: cannot access '");
        terminal_writestring(target_path);
        terminal_writestring("': No such directory\n");
        return;
    }
}
