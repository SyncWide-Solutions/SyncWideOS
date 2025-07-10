#include "../include/filesystem.h"
#include "../include/string.h"
#include "../include/vga.h"
#include <stdint.h>

extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);

void cmd_write(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    if (*args == '\0') {
        terminal_writestring("Usage: write <filename> <content>\n");
        terminal_writestring("   or: write <filename> (for interactive mode)\n");
        return;
    }
    
    // Find the filename (first argument)
    const char* filename_start = args;
    const char* filename_end = args;
    
    // Find end of filename (space or end of string)
    while (*filename_end && *filename_end != ' ') {
        filename_end++;
    }
    
    // Extract filename
    char filename[256];
    size_t filename_len = filename_end - filename_start;
    if (filename_len >= sizeof(filename)) {
        terminal_writestring("Error: Filename too long\n");
        return;
    }
    
    strncpy(filename, filename_start, filename_len);
    filename[filename_len] = '\0';
    
    // Skip spaces after filename
    const char* content = filename_end;
    while (*content == ' ') content++;
    
    if (fs_is_mounted()) {
        // Use FAT32 filesystem
        fs_file_handle_t* file = fs_open(filename, "w");
        if (!file) {
            terminal_writestring("Error: Could not create file '");
            terminal_writestring(filename);
            terminal_writestring("'\n");
            return;
        }
        
        if (*content) {
            // Write provided content
            size_t bytes_written = fs_write(file, content, strlen(content));
            fs_close(file);
            
            terminal_writestring("Wrote ");
            // Simple number to string conversion
            char num_str[16];
            int pos = 0;
            size_t num = bytes_written;
            if (num == 0) {
                num_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (num > 0) {
                    temp[temp_pos++] = '0' + (num % 10);
                    num /= 10;
                }
                for (int i = temp_pos - 1; i >= 0; i--) {
                    num_str[pos++] = temp[i];
                }
            }
            num_str[pos] = '\0';
            
            terminal_writestring(num_str);
            terminal_writestring(" bytes to '");
            terminal_writestring(filename);
            terminal_writestring("'\n");
        } else {
            // Interactive mode - read from user input
            terminal_writestring("Enter content (type 'EOF' on a new line to finish):\n");
            
            char buffer[4096];
            size_t buffer_pos = 0;
            char line[256];
            
            // Simple input reading (this would need proper keyboard handling)
            terminal_writestring("Interactive mode not fully implemented yet.\n");
            terminal_writestring("Use: write filename \"content here\"\n");
            
            fs_close(file);
        }
    } else {
        // Use legacy filesystem
        fs_node_t* parent = fs_get_cwd();
        
        // Check if file already exists
        fs_node_t* existing_file = fs_find_node(parent, filename);
        if (!existing_file) {
            // Create new file
            existing_file = fs_create_file(parent, filename);
            if (!existing_file) {
                terminal_writestring("Error: Could not create file '");
                terminal_writestring(filename);
                terminal_writestring("'\n");
                return;
            }
        }
        
        if (*content) {
            // Write content to file
            if (fs_write_file(existing_file, content)) {
                terminal_writestring("Content written to '");
                terminal_writestring(filename);
                terminal_writestring("'\n");
            } else {
                terminal_writestring("Error: Could not write to file '");
                terminal_writestring(filename);
                terminal_writestring("'\n");
            }
        } else {
            terminal_writestring("Interactive mode not available in legacy filesystem.\n");
            terminal_writestring("Use: write filename \"content here\"\n");
        }
    }
}