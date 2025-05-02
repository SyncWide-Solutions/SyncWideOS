#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdbool.h>
#include <stddef.h>

#define FS_MAX_FILES 64
#define FS_MAX_NAME_LENGTH 32
#define FS_MAX_PATH_LENGTH 256
#define FS_MAX_CONTENT_SIZE 4096

#define FS_TYPE_FILE 0
#define FS_TYPE_DIRECTORY 1

// Forward declaration
typedef struct fs_node fs_node_t;

// File system node structure
struct fs_node {
    char name[FS_MAX_NAME_LENGTH];
    int type;
    size_t size;
    fs_node_t* parent;
    fs_node_t* children[FS_MAX_FILES];
    size_t child_count;
    char content[FS_MAX_CONTENT_SIZE];
};

// Filesystem function prototypes
void fs_init(void);
fs_node_t* fs_get_root(void);
fs_node_t* fs_get_cwd(void);
void fs_set_cwd(fs_node_t* dir);
fs_node_t* fs_create_file(fs_node_t* dir, const char* name);
fs_node_t* fs_create_directory(fs_node_t* dir, const char* name);
fs_node_t* fs_find_node(fs_node_t* dir, const char* name);
fs_node_t* fs_find_node_by_path(const char* path);
bool fs_write_file(fs_node_t* file, const char* content);
void fs_get_path(fs_node_t* node, char* path_buffer, size_t buffer_size);

#endif /* FILESYSTEM_H */
