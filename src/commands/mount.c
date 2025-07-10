#include "../include/filesystem.h"
#include "../include/commands.h"

extern void terminal_writestring(const char* data);

void cmd_mount(const char* args) {
    (void)args; // Unused parameter
    
    if (fs_is_mounted()) {
        terminal_writestring("FAT32 filesystem is already mounted\n");
        return;
    }
    
    if (fs_mount()) {
        terminal_writestring("FAT32 filesystem mounted successfully\n");
    } else {
        terminal_writestring("Failed to mount FAT32 filesystem\n");
    }
}

void cmd_unmount(const char* args) {
    (void)args; // Unused parameter
    
    if (!fs_is_mounted()) {
        terminal_writestring("No filesystem is currently mounted\n");
        return;
    }
    
    fs_unmount();
    terminal_writestring("FAT32 filesystem unmounted\n");
}
