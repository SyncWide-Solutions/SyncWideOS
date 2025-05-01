#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stddef.h>
#include <stdbool.h>

#define FS_MAX_NAME_LENGTH 32
#define FS_MAX_PATH_LENGTH 256
#define FS_MAX_FILES 64
#define FS_MAX_CONTENT_SIZE 1024

typedef enum {
    FS_TYPE_FILE,
    FS_TYPE_DIRECTORY
} fs_node_type_t;

typedef struct fs_node {
    char name[FS_MAX_NAME_LENGTH];
    fs_node_type_t type;
    size_t size;
    struct fs_node* parent;
    
    // For directories: children nodes
    struct fs_node* children[FS_MAX_FILES];
    size_t child_count;
    
    // For files: content
    char content[FS_MAX_CONTENT_SIZE];
} fs_node_t;

// Initialize the filesystem
void fs_init(void);

// Get the root directory node
fs_node_t* fs_get_root(void);

// Get the current working directory
fs_node_t* fs_get_cwd(void);

// Set the current working directory
void fs_set_cwd(fs_node_t* dir);

// Create a new file in the specified directory
fs_node_t* fs_create_file(fs_node_t* dir, const char* name);

// Create a new directory in the specified directory
fs_node_t* fs_create_directory(fs_node_t* dir, const char* name);

// Find a node by name in the specified directory
fs_node_t* fs_find_node(fs_node_t* dir, const char* name);

// Find a node by path (absolute or relative)
fs_node_t* fs_find_node_by_path(const char* path);

// Write content to a file
bool fs_write_file(fs_node_t* file, const char* content);

// Get the full path of a node
void fs_get_path(fs_node_t* node, char* path_buffer, size_t buffer_size);

#endif /* FILESYSTEM_H */
