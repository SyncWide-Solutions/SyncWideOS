#include "../include/filesystem.h"
#include "../include/string.h"
#include <string.h>

extern void terminal_writestring(const char* data);

void cmd_mkdir(const char* args) {
    if (!args || !*args) {
        terminal_writestring("mkdir: missing operand\n");
        return;
    }
    
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check for empty argument after spaces
    if (!*args) {
        terminal_writestring("mkdir: missing operand\n");
        return;
    }
    
    // Find the last slash to separate path and directory name
    const char* last_slash = NULL;
    const char* ptr = args;
    
    while (*ptr) {
        if (*ptr == '/') {
            last_slash = ptr;
        }
        ptr++;
    }
    
    fs_node_t* parent;
    char dir_name[FS_MAX_NAME_LENGTH];
    
    if (!last_slash) {
        // No slashes in path, create in current directory
        parent = fs_get_cwd();
        strncpy(dir_name, args, FS_MAX_NAME_LENGTH - 1);
        dir_name[FS_MAX_NAME_LENGTH - 1] = '\0';
    } else {
        // Extract parent path and directory name
        char parent_path[FS_MAX_PATH_LENGTH];
        size_t path_len = last_slash - args;
        
        // Handle case where path is just "/"
        if (path_len == 0 && last_slash == args) {
            parent = fs_get_root();
        } else {
            // Copy the parent path
            if (path_len >= FS_MAX_PATH_LENGTH) {
                terminal_writestring("mkdir: path too long\n");
                return;
            }
            
            strncpy(parent_path, args, path_len);
            parent_path[path_len] = '\0';
            
            parent = fs_find_node_by_path(parent_path);
        }
        
        if (!parent || parent->type != FS_TYPE_DIRECTORY) {
            terminal_writestring("mkdir: cannot create directory '");
            terminal_writestring(args);
            terminal_writestring("': No such parent directory\n");
            return;
        }
        
        // Copy the directory name
        strncpy(dir_name, last_slash + 1, FS_MAX_NAME_LENGTH - 1);
        dir_name[FS_MAX_NAME_LENGTH - 1] = '\0';
    }
    
    if (!*dir_name) {
        terminal_writestring("mkdir: cannot create directory with empty name\n");
        return;
    }
    
    // Create the directory
    fs_node_t* new_dir = fs_create_directory(parent, dir_name);
    
    if (!new_dir) {
        terminal_writestring("mkdir: cannot create directory '");
        terminal_writestring(args);
        terminal_writestring("': Directory already exists or filesystem full\n");
    }
}
