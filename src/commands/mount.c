#include "../include/filesystem.h"
#include "../include/commands.h"

extern void terminal_writestring(const char* data);

void cmd_mount(const char* args) {
    if (!args || !*args) {
        // Try to mount FAT32
        if (fs_mount()) {
            terminal_writestring("FAT32 filesystem mounted successfully\n");
        } else {
            terminal_writestring("Failed to mount FAT32 filesystem\n");
        }
        return;
    }
    
    // Handle specific mount options if needed
    terminal_writestring("Mount command executed\n");
}

void cmd_unmount(const char* args) {
    (void)args; // Unused parameter
    
    if (fs_is_mounted()) {
        fs_unmount();
        terminal_writestring("FAT32 filesystem unmounted\n");
        terminal_writestring("Legacy filesystem is now active\n");
    } else {
        terminal_writestring("No filesystem is currently mounted\n");
    }
}
