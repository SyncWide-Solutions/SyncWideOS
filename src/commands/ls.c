#include "../include/vga.h"
#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);

// Callback function for directory listing
static void ls_callback(const char* name, bool is_dir, uint32_t size) {
    // Set color based on file type
    if (is_dir) {
        terminal_setcolor(vga_entry_color(VGA_COLOR_BLUE, VGA_COLOR_BLACK));
        terminal_writestring(name);
        terminal_writestring("/");
    } else {
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        terminal_writestring(name);
    }
    
    terminal_writestring("  ");
    
    // Reset color
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
}

void cmd_ls(const char* args) {
    // Check if FAT32 is mounted
    if (fs_is_mounted()) {
        // Use FAT32 filesystem
        const char* path = NULL;
        
        // Skip leading spaces
        while (args && *args == ' ') args++;
        
        if (args && *args) {
            path = args;
        }
        
        // List directory contents
        if (fs_list_directory(path, ls_callback)) {
            terminal_writestring("\n");
        } else {
            terminal_writestring("ls: cannot access directory\n");
        }
        return;
    }
    
    // Fallback message if FAT32 is not mounted
    terminal_writestring("ls: FAT32 filesystem not mounted\n");
}
