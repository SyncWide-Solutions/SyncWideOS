#include "../include/filesystem.h"
#include "../include/string.h"

// Global filesystem state
static fs_node_t fs_nodes[FS_MAX_FILES];
static size_t fs_node_count = 0;
static fs_node_t* fs_root = NULL;
static fs_node_t* fs_cwd = NULL;

void fs_init(void) {
    // Reset all nodes
    for (size_t i = 0; i < FS_MAX_FILES; i++) {
        fs_nodes[i].name[0] = '\0';
        fs_nodes[i].type = FS_TYPE_FILE;
        fs_nodes[i].size = 0;
        fs_nodes[i].parent = NULL;
        fs_nodes[i].child_count = 0;
        fs_nodes[i].content[0] = '\0';
    }
    
    fs_node_count = 0;
    
    // Create root directory
    fs_root = &fs_nodes[fs_node_count++];
    strcpy(fs_root->name, "root");
    fs_root->type = FS_TYPE_DIRECTORY;
    fs_root->parent = NULL;
    
    // Set current working directory to root
    fs_cwd = fs_root;
    
    // Create some initial directories
    fs_create_directory(fs_root, "bin");
    fs_create_directory(fs_root, "home");
    fs_create_directory(fs_root, "etc");
    
    // Create a welcome file
    fs_node_t* welcome = fs_create_file(fs_root, "welcome.txt");
    fs_write_file(welcome, "Welcome to SyncWideOS Filesystem!\nType 'help' for available commands.");

    // Create a sample image file
    fs_create_sample_images();
}

fs_node_t* fs_get_root(void) {
    return fs_root;
}

fs_node_t* fs_get_cwd(void) {
    return fs_cwd;
}

void fs_set_cwd(fs_node_t* dir) {
    if (dir && dir->type == FS_TYPE_DIRECTORY) {
        fs_cwd = dir;
    }
}

fs_node_t* fs_create_file(fs_node_t* dir, const char* name) {
    if (!dir || dir->type != FS_TYPE_DIRECTORY || dir->child_count >= FS_MAX_FILES || fs_node_count >= FS_MAX_FILES) {
        return NULL;
    }
    
    // Check if file already exists
    if (fs_find_node(dir, name) != NULL) {
        return NULL;
    }
    
    // Create new file
    fs_node_t* file = &fs_nodes[fs_node_count++];
    strcpy(file->name, name);
    file->type = FS_TYPE_FILE;
    file->size = 0;
    file->parent = dir;
    file->content[0] = '\0';
    
    // Add to parent directory
    dir->children[dir->child_count++] = file;
    
    return file;
}

void fs_create_sample_images(void) {
    // Create a simple gradient image
    fs_node_t* gradient = fs_create_file(fs_root, "gradient.img");
    if (gradient) {
        // Create a simple 10x10 grayscale gradient pattern
        char gradient_data[FS_MAX_CONTENT_SIZE] = 
            "0123456789\n"
            "1234567890\n"
            "2345678901\n"
            "3456789012\n"
            "4567890123\n"
            "5678901234\n"
            "6789012345\n"
            "7890123456\n"
            "8901234567\n"
            "9012345678\n";
        
        fs_write_file(gradient, gradient_data);
    }
    
    // Create a colorful rainbow pattern
    fs_node_t* rainbow = fs_create_file(fs_root, "rainbow.img");
    if (rainbow) {
        // Each row uses a different color from the VGA palette
        // We'll use the same intensity (9) but different colors
        char rainbow_data[FS_MAX_CONTENT_SIZE] = 
            "4444444444\n"  // Red (VGA_COLOR_RED = 4)
            "6666666666\n"  // Brown (VGA_COLOR_BROWN = 6)
            "2222222222\n"  // Green (VGA_COLOR_GREEN = 2)
            "3333333333\n"  // Cyan (VGA_COLOR_CYAN = 3)
            "1111111111\n"  // Blue (VGA_COLOR_BLUE = 1)
            "5555555555\n"  // Magenta (VGA_COLOR_MAGENTA = 5)
            "8888888888\n"  // Dark Grey (VGA_COLOR_DARK_GREY = 8)
            "7777777777\n"  // Light Grey (VGA_COLOR_LIGHT_GREY = 7)
            "9999999999\n"  // Light Blue (VGA_COLOR_LIGHT_BLUE = 9)
            "AAAAAAAAAA\n"; // Light Green (VGA_COLOR_LIGHT_GREEN = 10, 'A' in hex)
        
        fs_write_file(rainbow, rainbow_data);
    }
    
    // Create a colorful SyncWide logo
    fs_node_t* logo = fs_create_file(fs_root, "logo.bmp");
    if (logo) {
        // Create a simple logo with different colors
        // Using hex values that correspond to VGA colors
        char logo_data[FS_MAX_CONTENT_SIZE] = 
            "000000000000000000000\n"
            "000111111111111110000\n"
            "001444444444444441000\n"
            "014444444444444444100\n"
            "014422222222222441000\n"
            "014422222222224410000\n"
            "014422222222244100000\n"
            "014422222222441000000\n"
            "014422222224410000000\n"
            "014422222244100000000\n"
            "014422222441000000000\n"
            "014422224410000000000\n"
            "014422244100000000000\n"
            "014422441000000000000\n"
            "014424410000000000000\n"
            "014444100000000000000\n"
            "014441000000000000000\n"
            "001410000000000000000\n"
            "000100000000000000000\n"
            "000000000000000000000\n";
        
        fs_write_file(logo, logo_data);
    }
    
    // Create a checkerboard pattern
    fs_node_t* checkerboard = fs_create_file(fs_root, "checker.pic");
    if (checkerboard) {
        // Alternating black and white squares
        char checker_data[FS_MAX_CONTENT_SIZE] = 
            "0F0F0F0F0F\n"
            "F0F0F0F0F0\n"
            "0F0F0F0F0F\n"
            "F0F0F0F0F0\n"
            "0F0F0F0F0F\n"
            "F0F0F0F0F0\n"
            "0F0F0F0F0F\n"
            "F0F0F0F0F0\n"
            "0F0F0F0F0F\n"
            "F0F0F0F0F0\n";
        
        fs_write_file(checkerboard, checker_data);
    }
}

fs_node_t* fs_create_directory(fs_node_t* dir, const char* name) {
    if (!dir || dir->type != FS_TYPE_DIRECTORY || dir->child_count >= FS_MAX_FILES || fs_node_count >= FS_MAX_FILES) {
        return NULL;
    }
    
    // Check if directory already exists
    if (fs_find_node(dir, name) != NULL) {
        return NULL;
    }
    
    // Create new directory
    fs_node_t* new_dir = &fs_nodes[fs_node_count++];
    strcpy(new_dir->name, name);
    new_dir->type = FS_TYPE_DIRECTORY;
    new_dir->size = 0;
    new_dir->parent = dir;
    new_dir->child_count = 0;
    
    // Add to parent directory
    dir->children[dir->child_count++] = new_dir;
    
    return new_dir;
}

fs_node_t* fs_find_node(fs_node_t* dir, const char* name) {
    if (!dir || dir->type != FS_TYPE_DIRECTORY) {
        return NULL;
    }
    
    for (size_t i = 0; i < dir->child_count; i++) {
        if (strcmp(dir->children[i]->name, name) == 0) {
            return dir->children[i];
        }
    }
    
    return NULL;
}

fs_node_t* fs_find_node_by_path(const char* path) {
    if (!path || !*path) {
        return NULL;
    }
    
    // Handle absolute paths
    fs_node_t* current;
    if (path[0] == '/') {
        current = fs_root;
        path++; // Skip the leading '/'
    } else if (path[0] == '~') {
        current = fs_root;
        path++; // Skip the '~'
        if (path[0] == '/') {
            path++; // Skip the '/' after '~'
        }
    } else {
        current = fs_cwd;
    }
    
    // Handle empty path or just "~"
    if (!*path) {
        return current;
    }
    
    char component[FS_MAX_NAME_LENGTH];
    size_t i = 0;
    
    while (*path) {
        // Extract next path component
        i = 0;
        while (*path && *path != '/' && i < FS_MAX_NAME_LENGTH - 1) {
            component[i++] = *path++;
        }
        component[i] = '\0';
        
        // Skip consecutive slashes
        while (*path == '/') {
            path++;
        }
        
        // Handle special directory names
        if (strcmp(component, ".") == 0) {
            // Current directory - do nothing
        } else if (strcmp(component, "..") == 0) {
            // Parent directory
            if (current->parent) {
                current = current->parent;
            }
        } else {
            // Regular directory or file
            current = fs_find_node(current, component);
            if (!current) {
                return NULL; // Path component not found
            }
            
            // If we found a file but there's more path to process, it's an error
            if (current->type == FS_TYPE_FILE && *path) {
                return NULL;
            }
        }
    }
    
    return current;
}

bool fs_write_file(fs_node_t* file, const char* content) {
    if (!file || file->type != FS_TYPE_FILE) {
        return false;
    }
    
    size_t content_len = strlen(content);
    if (content_len >= FS_MAX_CONTENT_SIZE) {
        return false; // Content too large
    }
    
    strcpy(file->content, content);
    file->size = content_len;
    
    return true;
}

void fs_get_path(fs_node_t* node, char* path_buffer, size_t buffer_size) {
    if (!node || !path_buffer || buffer_size == 0) {
        return;
    }
    
    // Special case for root
    if (node == fs_root) {
        strcpy(path_buffer, "~");
        return;
    }
    
    // Build path by traversing up to root
    char temp_path[FS_MAX_PATH_LENGTH] = {0};
    size_t offset = 0;
    
    // Start with a tilde for home directory
    temp_path[offset++] = '~';
    
    // Create an array to store pointers to each directory name
    fs_node_t* path_nodes[FS_MAX_PATH_LENGTH / 2]; // Conservative estimate
    int path_depth = 0;
    
    // Traverse up the directory tree and store each node
    fs_node_t* current = node;
    while (current && current != fs_root) {
        path_nodes[path_depth++] = current;
        current = current->parent;
    }
    
    // Now build the path from root to the current directory
    for (int i = path_depth - 1; i >= 0; i--) {
        temp_path[offset++] = '/';
        
        // Copy the directory name
        const char* name = path_nodes[i]->name;
        size_t name_len = strlen(name);
        
        for (size_t j = 0; j < name_len && offset < buffer_size - 1; j++) {
            temp_path[offset++] = name[j];
        }
    }
    
    // Null terminate
    temp_path[offset] = '\0';
    
    // Copy to the output buffer
    strncpy(path_buffer, temp_path, buffer_size - 1);
    path_buffer[buffer_size - 1] = '\0';
}
