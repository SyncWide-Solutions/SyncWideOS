#include "../include/vga.h"
#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);

void cmd_ls(const char* args) {
    // Check if FAT32 is mounted
    if (fs_is_mounted()) {
        // Use FAT32 filesystem
        terminal_writestring("FAT32 files:\n");
        // You could implement FAT32 directory listing here
        // For now, just show that FAT32 is active
        terminal_writestring("TEST.TXT\n");
        return;
    }
    
    // Use legacy filesystem
    fs_node_t* dir;
    
    // Skip leading spaces
    while (args && *args == ' ') args++;
    
    if (!args || !*args) {
        // No arguments, list current directory
        dir = fs_get_cwd();
    } else {
        // List the specified directory
        dir = fs_find_node_by_path(args);
    }
    
    if (!dir || dir->type != FS_TYPE_DIRECTORY) {
        terminal_writestring("ls: cannot access '");
        if (args && *args) terminal_writestring(args);
        else terminal_writestring("current directory");
        terminal_writestring("': No such directory\n");
        return;
    }
    
    // Print directory contents
    for (size_t i = 0; i < dir->child_count; i++) {
        fs_node_t* node = dir->children[i];
        
        // Set color based on node type
        if (node->type == FS_TYPE_DIRECTORY) {
            terminal_setcolor(vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_BLACK));
            terminal_writestring(node->name);
            terminal_writestring("/");
        } else {
            terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
            terminal_writestring(node->name);
        }
        
        terminal_writestring("  ");
    }
    
    // Reset color
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    terminal_writestring("\n");
}
