#include "../include/filesystem.h"
#include "../include/string.h"
#include "../include/vga.h"

extern void terminal_writestring(const char* data);

// Helper function to find substring manually
static const char* find_redirect(const char* str, const char* pattern) {
    if (!str || !pattern) return NULL;
    
    size_t pattern_len = strlen(pattern);
    size_t str_len = strlen(str);
    
    if (pattern_len > str_len) return NULL;
    
    for (size_t i = 0; i <= str_len - pattern_len; i++) {
        if (strncmp(str + i, pattern, pattern_len) == 0) {
            return str + i;
        }
    }
    
    return NULL;
}

void cmd_echo(const char* args) {
    if (!args || !*args) {
        terminal_writestring("\n");
        return;
    }
    
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check for output redirection
    const char* redirect_pos = find_redirect(args, " > ");
    const char* append_pos = find_redirect(args, " >> ");
    
    if (append_pos) {
        // Append to file
        size_t content_len = append_pos - args;
        char content[1024];
        
        if (content_len >= sizeof(content)) {
            terminal_writestring("Error: Content too long\n");
            return;
        }
        
        strncpy(content, args, content_len);
        content[content_len] = '\0';
        
        // Get filename
        const char* filename = append_pos + 4; // Skip " >> "
        while (*filename == ' ') filename++;
        
        if (!*filename) {
            terminal_writestring("Error: No filename specified\n");
            return;
        }
        
        // Extract just the filename (stop at space or end)
        char filename_clean[256];
        size_t i = 0;
        while (filename[i] && filename[i] != ' ' && i < sizeof(filename_clean) - 1) {
            filename_clean[i] = filename[i];
            i++;
        }
        filename_clean[i] = '\0';
        
        if (fs_is_mounted()) {
            fs_file_handle_t* file = fs_open(filename_clean, "a");
            if (!file) {
                terminal_writestring("Error: Could not open file for appending\n");
                return;
            }
            
            fs_write(file, content, strlen(content));
            fs_write(file, "\n", 1);
            fs_close(file);
            
            terminal_writestring("Content appended to '");
            terminal_writestring(filename_clean);
            terminal_writestring("'\n");
        } else {
            terminal_writestring("File redirection requires mounted filesystem\n");
        }
    } else if (redirect_pos) {
        // Write to file (overwrite)
        size_t content_len = redirect_pos - args;
        char content[1024];
        
        if (content_len >= sizeof(content)) {
            terminal_writestring("Error: Content too long\n");
            return;
        }
        
        strncpy(content, args, content_len);
        content[content_len] = '\0';
        
        // Get filename
        const char* filename = redirect_pos + 3; // Skip " > "
        while (*filename == ' ') filename++;
        
        if (!*filename) {
            terminal_writestring("Error: No filename specified\n");
            return;
        }
        
        // Extract just the filename (stop at space or end)
        char filename_clean[256];
        size_t i = 0;
        while (filename[i] && filename[i] != ' ' && i < sizeof(filename_clean) - 1) {
            filename_clean[i] = filename[i];
            i++;
        }
        filename_clean[i] = '\0';
        
        if (fs_is_mounted()) {
            fs_file_handle_t* file = fs_open(filename_clean, "w");
            if (!file) {
                terminal_writestring("Error: Could not create file\n");
                return;
            }
            
            fs_write(file, content, strlen(content));
            fs_write(file, "\n", 1);
            fs_close(file);
            
            terminal_writestring("Content written to '");
            terminal_writestring(filename_clean);
            terminal_writestring("'\n");
        } else {
            terminal_writestring("File redirection requires mounted filesystem\n");
        }
    } else {
        // Normal echo to terminal
        terminal_writestring(args);
        terminal_writestring("\n");
    }
}
