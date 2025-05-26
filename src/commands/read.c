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
    // This is a simplified renderer that assumes the image content
    // contains special formatting with pixel data
    
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
    if (fs_is_mounted()) {
        // Try to read from FAT32 filesystem
        fs_file_handle_t* handle = fs_open(args, "r");  // Added mode parameter
        if (!handle) {
            terminal_writestring("Error: File not found in FAT32 filesystem: ");
            terminal_writestring(args);
            terminal_writestring("\n");
            return;
        }
        
        // Read file content
        char buffer[1024];
        size_t bytes_read = fs_read(handle, buffer, sizeof(buffer) - 1);
        buffer[bytes_read] = '\0';
        
        terminal_writestring("--- File: ");
        terminal_writestring(args);
        terminal_writestring(" (FAT32) ---\n");
        terminal_writestring(buffer);
        terminal_writestring("\n--- End of file ---\n");
        
        fs_close(handle);
        return;
    }
    
    // Use legacy filesystem
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
    terminal_writestring(" (Legacy) ---\n");
    
    // Check if it's an image file
    if (is_image_file(args)) {
        terminal_writestring("Detected image file. Rendering...\n");
        render_image_ascii(file->content);
    } else {
        // Regular text file
        terminal_writestring(file->content);
    }
    
    terminal_writestring("\n--- End of file ---\n");
}
