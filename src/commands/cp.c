#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

void cmd_cp(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    if (*args == '\0') {
        terminal_writestring("Usage: cp <source> <destination>\n");
        return;
    }
    
    // Parse source filename
    const char* src_start = args;
    const char* src_end = args;
    while (*src_end && *src_end != ' ') {
        src_end++;
    }
    
    if (*src_end == '\0') {
        terminal_writestring("Usage: cp <source> <destination>\n");
        return;
    }
    
    // Extract source filename
    char src_filename[256];
    size_t src_len = src_end - src_start;
    if (src_len >= sizeof(src_filename)) {
        terminal_writestring("Error: Source filename too long\n");
        return;
    }
    strncpy(src_filename, src_start, src_len);
    src_filename[src_len] = '\0';
    
    // Skip spaces and get destination
    const char* dst_start = src_end;
    while (*dst_start == ' ') dst_start++;
    
    if (*dst_start == '\0') {
        terminal_writestring("Usage: cp <source> <destination>\n");
        return;
    }
    
    // Extract destination filename
    const char* dst_end = dst_start;
    while (*dst_end && *dst_end != ' ') {
        dst_end++;
    }
    
    char dst_filename[256];
    size_t dst_len = dst_end - dst_start;
    if (dst_len >= sizeof(dst_filename)) {
        terminal_writestring("Error: Destination filename too long\n");
        return;
    }
    strncpy(dst_filename, dst_start, dst_len);
    dst_filename[dst_len] = '\0';
    
    if (fs_is_mounted()) {
        // Check if source exists
        if (!fs_exists(src_filename)) {
            terminal_writestring("Error: Source file '");
            terminal_writestring(src_filename);
            terminal_writestring("' not found\n");
            return;
        }
        
        // Check if destination already exists
        if (fs_exists(dst_filename)) {
            terminal_writestring("Error: Destination file '");
            terminal_writestring(dst_filename);
            terminal_writestring("' already exists\n");
            return;
        }
        
        // Open source file
        fs_file_handle_t* src_file = fs_open(src_filename, "r");
        if (!src_file) {
            terminal_writestring("Error: Could not open source file\n");
            return;
        }
        
        // Create destination file
        fs_file_handle_t* dst_file = fs_open(dst_filename, "w");
        if (!dst_file) {
            terminal_writestring("Error: Could not create destination file\n");
            fs_close(src_file);
            return;
        }
        
        // Copy data
        char buffer[512];
        size_t bytes_read;
        size_t total_copied = 0;
        
        while ((bytes_read = fs_read(src_file, buffer, sizeof(buffer))) > 0) {
            size_t bytes_written = fs_write(dst_file, buffer, bytes_read);
            total_copied += bytes_written;
            
            if (bytes_written != bytes_read) {
                terminal_writestring("Error: Write failed during copy\n");
                break;
            }
        }
        
        fs_close(src_file);
        fs_close(dst_file);
        
        terminal_writestring("Copied ");
        // Simple number to string conversion
        char num_str[16];
        int pos = 0;
        size_t num = total_copied;
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
        terminal_writestring(" bytes from '");
        terminal_writestring(src_filename);
        terminal_writestring("' to '");
        terminal_writestring(dst_filename);
        terminal_writestring("'\n");
    } else {
                // Use legacy filesystem
        fs_node_t* src_file = fs_find_node_by_path(src_filename);
        if (!src_file || src_file->type != FS_TYPE_FILE) {
            terminal_writestring("Error: Source file not found or not a file\n");
            return;
        }
        
        // Check if destination already exists
        if (fs_find_node_by_path(dst_filename)) {
            terminal_writestring("Error: Destination file already exists\n");
            return;
        }
        
        // Create destination file
        fs_node_t* parent = fs_get_cwd();
        fs_node_t* dst_file = fs_create_file(parent, dst_filename);
        if (!dst_file) {
            terminal_writestring("Error: Could not create destination file\n");
            return;
        }
        
        // Copy content
        if (fs_write_file(dst_file, src_file->content)) {
            terminal_writestring("File copied successfully from '");
            terminal_writestring(src_filename);
            terminal_writestring("' to '");
            terminal_writestring(dst_filename);
            terminal_writestring("'\n");
        } else {
            terminal_writestring("Error: Failed to copy file content\n");
        }
    }
}
