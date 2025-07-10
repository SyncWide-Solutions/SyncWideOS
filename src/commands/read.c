#include "../include/filesystem.h"
#include "../include/string.h"
#include "../include/vga.h"
#include <stdint.h>

extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);
extern uint8_t terminal_getcolor(void);

// Define image file extensions
const char* IMAGE_EXTENSIONS[] = {".img", ".bmp", ".pic", NULL};

// Check if a filename has an image extension
int is_image_file(const char* filename) {
    int i = 0;
    int len = strlen(filename);
    
    while (IMAGE_EXTENSIONS[i] != NULL) {
        int ext_len = strlen(IMAGE_EXTENSIONS[i]);
        if (len > ext_len && strcmp(filename + len - ext_len, IMAGE_EXTENSIONS[i]) == 0) {
            return 1;
        }
        i++;
    }
    return 0;
}

// Simple ASCII art renderer for image files
void render_image_ascii(const char* content) {
    terminal_writestring("ASCII Image Rendering:\n\n");
    
    uint8_t original_color = terminal_getcolor();
    
    // Process the image data
    const char* ptr = content;
    while (*ptr) {
        if ((*ptr >= '0' && *ptr <= '9') || (*ptr >= 'A' && *ptr <= 'F')) {
            // Convert hex character to a value (0-15)
            int value;
            if (*ptr >= '0' && *ptr <= '9') {
                value = *ptr - '0';
            } else {
                value = (*ptr - 'A') + 10;
            }
            
            // Use the value directly as a VGA color
            uint8_t color = vga_entry_color(VGA_COLOR_WHITE, value);
            terminal_setcolor(color);
            terminal_writestring("█"); // Full block character
        } 
        else if (*ptr == '\n') {
            terminal_writestring("\n");
        }
        ptr++;
    }
    
    // Restore original color
        terminal_setcolor(original_color);
}

void cmd_read(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check if a filename was provided
    if (*args == '\0') {
        terminal_writestring("Usage: read <filename>\n");
        return;
    }
    
    // Check if FAT32 is mounted
    if (!fs_is_mounted()) {
        terminal_writestring("read: FAT32 filesystem not mounted\n");
        return;
    }
    
    // Try to read from FAT32 filesystem
    fs_file_handle_t* handle = fs_open(args, "r");
    if (!handle) {
        terminal_writestring("Error: File not found: ");
        terminal_writestring(args);
        terminal_writestring("\n");
        return;
    }
    
    if (handle->is_directory) {
        terminal_writestring("Error: Cannot read directory: ");
        terminal_writestring(args);
        terminal_writestring("\n");
        fs_close(handle);
        return;
    }
    
    // Read file content
    char buffer[1024];
    size_t total_read = 0;
    size_t bytes_read;
    
    terminal_writestring("--- File: ");
    terminal_writestring(args);
    terminal_writestring(" ---\n");
    
    // Check if it's an image file
    if (is_image_file(args)) {
        terminal_writestring("Detected image file. Rendering...\n");
        
        // Read entire file for image rendering
        char image_buffer[4096] = {0};
        size_t image_pos = 0;
        
        while ((bytes_read = fs_read(handle, buffer, sizeof(buffer))) > 0 && 
               image_pos < sizeof(image_buffer) - 1) {
            for (size_t i = 0; i < bytes_read && image_pos < sizeof(image_buffer) - 1; i++) {
                image_buffer[image_pos++] = buffer[i];
            }
        }
        image_buffer[image_pos] = '\0';
        
        render_image_ascii(image_buffer);
    } else {
        // Regular text file - read and display in chunks
        while ((bytes_read = fs_read(handle, buffer, sizeof(buffer) - 1)) > 0) {
            buffer[bytes_read] = '\0';
            terminal_writestring(buffer);
            total_read += bytes_read;
        }
    }
    
    terminal_writestring("\n--- End of file ---\n");
    fs_close(handle);
}
