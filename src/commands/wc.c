#include "../include/filesystem.h"
#include "../include/string.h"

extern void terminal_writestring(const char* data);

void cmd_wc(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    if (*args == '\0') {
        terminal_writestring("Usage: wc <filename>\n");
        return;
    }
    
    // Extract filename
    char filename[256];
    size_t i = 0;
    while (*args && *args != ' ' && i < sizeof(filename) - 1) {
        filename[i++] = *args++;
    }
    filename[i] = '\0';
    
    if (fs_is_mounted()) {
        // Check if file exists
        if (!fs_exists(filename)) {
            terminal_writestring("wc: ");
            terminal_writestring(filename);
            terminal_writestring(": No such file\n");
            return;
        }
        
        // Open and read file
        fs_file_handle_t* file = fs_open(filename, "r");
        if (!file) {
            terminal_writestring("wc: Cannot open ");
            terminal_writestring(filename);
            terminal_writestring("\n");
            return;
        }
        
        // Count lines, words, and characters
        char buffer[512];
        size_t bytes_read;
        size_t lines = 0;
        size_t words = 0;
        size_t chars = 0;
        bool in_word = false;
        
        while ((bytes_read = fs_read(file, buffer, sizeof(buffer))) > 0) {
            for (size_t j = 0; j < bytes_read; j++) {
                chars++;
                
                if (buffer[j] == '\n') {
                    lines++;
                }
                
                if (buffer[j] == ' ' || buffer[j] == '\t' || buffer[j] == '\n') {
                    if (in_word) {
                        words++;
                        in_word = false;
                    }
                } else {
                    in_word = true;
                }
            }
        }
        
        // Count last word if file doesn't end with whitespace
        if (in_word) {
            words++;
        }
        
        fs_close(file);
        
        // Display results
        char num_str[16];
        
        // Lines
        int pos = 0;
        size_t num = lines;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (num > 0) {
                temp[temp_pos++] = '0' + (num % 10);
                num /= 10;
            }
            for (int k = temp_pos - 1; k >= 0; k--) {
                num_str[pos++] = temp[k];
            }
        }
        num_str[pos] = '\0';
        terminal_writestring(num_str);
        terminal_writestring(" ");
        
        // Words
        pos = 0;
        num = words;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (num > 0) {
                temp[temp_pos++] = '0' + (num % 10);
                num /= 10;
            }
            for (int k = temp_pos - 1; k >= 0; k--) {
                num_str[pos++] = temp[k];
            }
        }
        num_str[pos] = '\0';
        terminal_writestring(num_str);
        terminal_writestring(" ");
        
        // Characters
        pos = 0;
        num = chars;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (num > 0) {
                temp[temp_pos++] = '0' + (num % 10);
                num /= 10;
            }
            for (int k = temp_pos - 1; k >= 0; k--) {
                num_str[pos++] = temp[k];
            }
        }
        num_str[pos] = '\0';
        terminal_writestring(num_str);
        terminal_writestring(" ");
        
        terminal_writestring(filename);
        terminal_writestring("\n");
        
    } else {
        // Use legacy filesystem
        fs_node_t* file = fs_find_node_by_path(filename);
        if (!file || file->type != FS_TYPE_FILE) {
            terminal_writestring("wc: ");
            terminal_writestring(filename);
            terminal_writestring(": No such file\n");
            return;
        }
        
        // Count in legacy file content
        const char* content = file->content;
        size_t lines = 0;
        size_t words = 0;
        size_t chars = strlen(content);
        bool in_word = false;
        
        for (size_t i = 0; i < chars; i++) {
            if (content[i] == '\n') {
                lines++;
            }
            
            if (content[i] == ' ' || content[i] == '\t' || content[i] == '\n') {
                if (in_word) {
                    words++;
                    in_word = false;
                }
            } else {
                in_word = true;
            }
        }
        
        if (in_word) {
            words++;
        }
        
        // Simple number output (reuse the same logic as above)
        char num_str[16];
        int pos;
        
        // Lines
        pos = 0;
        size_t num = lines;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (num > 0) {
                temp[temp_pos++] = '0' + (num % 10);
                num /= 10;
            }
            for (int k = temp_pos - 1; k >= 0; k--) {
                num_str[pos++] = temp[k];
            }
        }
        num_str[pos] = '\0';
        terminal_writestring(num_str);
        terminal_writestring(" ");
        
        // Words
        pos = 0;
        num = words;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (num > 0) {
                temp[temp_pos++] = '0' + (num % 10);
                num /= 10;
            }
            for (int k = temp_pos - 1; k >= 0; k--) {
                num_str[pos++] = temp[k];
            }
        }
        num_str[pos] = '\0';
        terminal_writestring(num_str);
        terminal_writestring(" ");
        
        // Characters
        pos = 0;
        num = chars;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (num > 0) {
                temp[temp_pos++] = '0' + (num % 10);
                num /= 10;
            }
            for (int k = temp_pos - 1; k >= 0; k--) {
                num_str[pos++] = temp[k];
            }
        }
        num_str[pos] = '\0';
        terminal_writestring(num_str);
        terminal_writestring(" ");
        
        terminal_writestring(filename);
        terminal_writestring("\n");
    }
}
