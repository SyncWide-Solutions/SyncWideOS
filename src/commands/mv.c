#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

void cmd_mv(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    if (*args == '\0') {
        terminal_writestring("Usage: mv <source> <destination>\n");
        return;
    }
    
    // Parse source filename
    const char* src_start = args;
    const char* src_end = args;
    while (*src_end && *src_end != ' ') {
        src_end++;
    }
    
    if (*src_end == '\0') {
        terminal_writestring("Usage: mv <source> <destination>\n");
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
        terminal_writestring("Usage: mv <source> <destination>\n");
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
            terminal_writestring("Error: Source '");
            terminal_writestring(src_filename);
            terminal_writestring("' not found\n");
            return;
        }
        
        // Check if destination already exists
        if (fs_exists(dst_filename)) {
            terminal_writestring("Error: Destination '");
            terminal_writestring(dst_filename);
            terminal_writestring("' already exists\n");
            return;
        }
        
        // Rename the file
        if (fs_rename(src_filename, dst_filename)) {
            terminal_writestring("Moved '");
            terminal_writestring(src_filename);
            terminal_writestring("' to '");
            terminal_writestring(dst_filename);
            terminal_writestring("'\n");
        } else {
            terminal_writestring("Error: Could not move file\n");
        }
    } else {
        // Use legacy filesystem
        fs_node_t* src_file = fs_find_node_by_path(src_filename);
        if (!src_file) {
            terminal_writestring("Error: Source not found\n");
            return;
        }
        
        // Check if destination already exists
        if (fs_find_node_by_path(dst_filename)) {
            terminal_writestring("Error: Destination already exists\n");
            return;
        }
        
        // Simple rename (just change the name)
        strncpy(src_file->name, dst_filename, FS_MAX_NAME_LENGTH - 1);
        src_file->name[FS_MAX_NAME_LENGTH - 1] = '\0';
        
        terminal_writestring("Moved '");
        terminal_writestring(src_filename);
        terminal_writestring("' to '");
        terminal_writestring(dst_filename);
        terminal_writestring("'\n");
    }
}