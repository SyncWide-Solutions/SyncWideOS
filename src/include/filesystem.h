#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// FAT32 constants
#define SECTOR_SIZE 512
#define FAT32_SIGNATURE 0xAA55
#define FAT32_FSINFO_SIGNATURE1 0x41615252
#define FAT32_FSINFO_SIGNATURE2 0x61417272
#define FAT32_FSINFO_SIGNATURE3 0xAA550000

// Directory entry attributes
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20
#define ATTR_LONG_NAME  (ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID)

// FAT32 special cluster values
#define FAT32_EOC       0x0FFFFFF8  // End of cluster chain
#define FAT32_BAD       0x0FFFFFF7  // Bad cluster
#define FAT32_FREE      0x00000000  // Free cluster

// Maximum path and name lengths
#define FS_MAX_PATH_LENGTH 260
#define FS_MAX_NAME_LENGTH 255

// Legacy filesystem constants (for backward compatibility)
#define FS_MAX_CONTENT_SIZE 4096
#define FS_MAX_CHILDREN 32
#define FS_MAX_FILES 256

// Legacy filesystem types (for backward compatibility)
typedef enum {
    FS_TYPE_FILE,
    FS_TYPE_DIRECTORY
} fs_node_type_t;

typedef struct fs_node {
    char name[FS_MAX_NAME_LENGTH];
    fs_node_type_t type;
    size_t size;
    struct fs_node* parent;
    struct fs_node* children[FS_MAX_CHILDREN];
    size_t child_count;
    char content[FS_MAX_CONTENT_SIZE];
} fs_node_t;

// FAT32 Boot Sector (BIOS Parameter Block)
typedef struct {
    uint8_t  jmp_boot[3];           // Jump instruction
    uint8_t  oem_name[8];           // OEM name
    uint16_t bytes_per_sector;      // Bytes per sector
    uint8_t  sectors_per_cluster;   // Sectors per cluster
    uint16_t reserved_sectors;      // Reserved sectors
    uint8_t  num_fats;              // Number of FATs
    uint16_t root_entries;          // Root directory entries (0 for FAT32)
    uint16_t total_sectors_16;      // Total sectors (0 for FAT32)
    uint8_t  media_type;            // Media descriptor
    uint16_t fat_size_16;           // FAT size in sectors (0 for FAT32)
    uint16_t sectors_per_track;     // Sectors per track
    uint16_t num_heads;             // Number of heads
    uint32_t hidden_sectors;        // Hidden sectors
    uint32_t total_sectors_32;      // Total sectors (FAT32)
    
    // FAT32 specific fields
    uint32_t fat_size_32;           // FAT size in sectors
    uint16_t ext_flags;             // Extended flags
    uint16_t fs_version;            // Filesystem version
    uint32_t root_cluster;          // Root directory cluster
    uint16_t fs_info;               // FSInfo sector
    uint16_t backup_boot_sector;    // Backup boot sector
    uint8_t  reserved[12];          // Reserved
    uint8_t  drive_number;          // Drive number
    uint8_t  reserved1;             // Reserved
    uint8_t  boot_signature;        // Boot signature
    uint32_t volume_id;             // Volume ID
    uint8_t  volume_label[11];      // Volume label
    uint8_t  fs_type[8];            // Filesystem type
    uint8_t  boot_code[420];        // Boot code
    uint16_t signature;             // Boot sector signature (0xAA55)
} __attribute__((packed)) fat32_boot_sector_t;

// FAT32 FSInfo structure
typedef struct {
    uint32_t lead_signature;        // 0x41615252
    uint8_t  reserved1[480];        // Reserved
    uint32_t struct_signature;      // 0x61417272
    uint32_t free_count;            // Free cluster count
    uint32_t next_free;             // Next free cluster
    uint8_t  reserved2[12];         // Reserved
    uint32_t trail_signature;       // 0xAA550000
} __attribute__((packed)) fat32_fsinfo_t;

// FAT32 Directory Entry (8.3 format)
typedef struct {
    uint8_t  name[11];              // Filename (8.3 format)
    uint8_t  attributes;            // File attributes
    uint8_t  nt_reserved;           // Reserved for NT
    uint8_t  creation_time_tenth;   // Creation time (tenths of second)
    uint16_t creation_time;         // Creation time
    uint16_t creation_date;         // Creation date
    uint16_t last_access_date;      // Last access date
    uint16_t first_cluster_high;    // High word of first cluster
    uint16_t write_time;            // Write time
    uint16_t write_date;            // Write date
    uint16_t first_cluster_low;     // Low word of first cluster
    uint32_t file_size;             // File size in bytes
} __attribute__((packed)) fat32_dir_entry_t;

// Long filename entry
typedef struct {
    uint8_t  order;                 // Order of this entry
    uint16_t name1[5];              // First 5 characters (UTF-16)
    uint8_t  attributes;            // Always ATTR_LONG_NAME
    uint8_t  type;                  // Always 0
    uint8_t  checksum;              // Checksum of short name
    uint16_t name2[6];              // Next 6 characters (UTF-16)
    uint16_t first_cluster;         // Always 0
    uint16_t name3[2];              // Last 2 characters (UTF-16)
} __attribute__((packed)) fat32_lfn_entry_t;

// File handle structure
typedef struct {
    bool     in_use;                // Is this handle in use?
    uint32_t first_cluster;         // First cluster of file
    uint32_t current_cluster;       // Current cluster
    uint32_t cluster_offset;        // Offset within current cluster
    uint32_t file_size;             // File size
    uint32_t position;              // Current position in file
    uint8_t  attributes;            // File attributes
    char     filename[FS_MAX_NAME_LENGTH]; // Filename
    bool     is_directory;          // Is this a directory?
} fs_file_handle_t;

// Filesystem state
typedef struct {
    bool     mounted;               // Is filesystem mounted?
    uint32_t fat_start_sector;      // First FAT sector
    uint32_t data_start_sector;     // First data sector
    uint32_t root_cluster;          // Root directory cluster
    uint32_t sectors_per_cluster;   // Sectors per cluster
    uint32_t bytes_per_cluster;     // Bytes per cluster
    uint32_t fat_size;              // FAT size in sectors
    uint32_t total_clusters;        // Total number of clusters
    uint32_t free_clusters;         // Number of free clusters
    uint32_t next_free_cluster;     // Next free cluster hint
    fat32_boot_sector_t boot_sector; // Boot sector
} fat32_fs_t;

// Current working directory structure
typedef struct {
    uint32_t cluster;               // Current directory cluster
    char path[FS_MAX_PATH_LENGTH];  // Current path string
} fs_cwd_t;

// Legacy filesystem interface (for backward compatibility)
fs_node_t* fs_create_directory(fs_node_t* parent, const char* name);
fs_node_t* fs_create_file(fs_node_t* parent, const char* name);
fs_node_t* fs_find_node(fs_node_t* parent, const char* name);
fs_node_t* fs_find_node_by_path(const char* path);
fs_node_t* fs_get_root(void);
fs_node_t* fs_get_cwd(void);
void fs_set_cwd(fs_node_t* dir);
void fs_get_path(fs_node_t* node, char* buffer, size_t buffer_size);
bool fs_write_file(fs_node_t* file, const char* content);
void fs_create_sample_images(void);

// FAT32 filesystem interface
bool fs_init(void);
bool fs_mount(void);
void fs_unmount(void);

// File operations
fs_file_handle_t* fs_open(const char* path, const char* mode);
void fs_close(fs_file_handle_t* handle);
size_t fs_read(fs_file_handle_t* handle, void* buffer, size_t size);
size_t fs_write(fs_file_handle_t* handle, const void* buffer, size_t size);
bool fs_seek(fs_file_handle_t* handle, uint32_t position);
uint32_t fs_tell(fs_file_handle_t* handle);

// Directory operations
bool fs_mkdir(const char* path);
bool fs_rmdir(const char* path);
bool fs_list_directory(const char* path, void (*callback)(const char* name, bool is_dir, uint32_t size));
bool fs_chdir(const char* path);
const char* fs_getcwd(void);

// File management
bool fs_delete(const char* path);
bool fs_rename(const char* old_path, const char* new_path);
bool fs_exists(const char* path);
uint32_t fs_get_file_size(const char* path);

// Utility functions
bool fs_is_mounted(void);
uint32_t fs_get_free_space(void);
uint32_t fs_get_total_space(void);

// Internal functions (for implementation)
uint32_t fat32_read_fat_entry(uint32_t cluster);
bool fat32_write_fat_entry(uint32_t cluster, uint32_t value);
uint32_t fat32_allocate_cluster(void);
bool fat32_free_cluster_chain(uint32_t start_cluster);
bool fat32_read_cluster(uint32_t cluster, void* buffer);
bool fat32_write_cluster(uint32_t cluster, const void* buffer);

// Storage interface (implemented by storage driver)
bool storage_read_sectors(uint32_t sector, uint32_t count, void* buffer);
bool storage_write_sectors(uint32_t sector, uint32_t count, const void* buffer);

#endif /* FILESYSTEM_H */
