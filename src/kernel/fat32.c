#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/string.h"

// External function declarations
extern void terminal_writestring(const char* data);

// FAT32 structures and constants
#define ATTR_ARCHIVE 0x20
#define FAT32_FSINFO_SIGNATURE1 0x41615252
#define FAT32_FSINFO_SIGNATURE2 0x61417272
#define FAT32_FSINFO_SIGNATURE3 0xAA550000
#define RAMDISK_SIZE (16 * 1024 * 1024) // 16MB

typedef struct {
    uint8_t jmp_boot[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t num_fats;
    uint16_t root_entries;
    uint16_t total_sectors_16;
    uint8_t media_type;
    uint16_t fat_size_16;
    uint16_t sectors_per_track;
    uint16_t num_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;
    uint32_t fat_size_32;
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t reserved[12];
    uint8_t drive_number;
    uint8_t reserved1;
    uint8_t boot_signature;
    uint32_t volume_id;
    uint8_t volume_label[11];
    uint8_t fs_type[8];
    uint8_t boot_code[420];
    uint16_t signature;
} __attribute__((packed)) fat32_boot_sector_t;

typedef struct {
    uint32_t lead_signature;
    uint8_t reserved1[480];
    uint32_t struct_signature;
    uint32_t free_count;
    uint32_t next_free;
    uint8_t reserved2[12];
    uint32_t trail_signature;
} __attribute__((packed)) fat32_fsinfo_t;

typedef struct {
    uint8_t name[11];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_write_time;
    uint16_t last_write_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed)) fat32_dir_entry_t;

typedef struct {
    const char* filename;
    const char* fat32_name;
    const char* content;
    size_t size;
} iso_file_data_t;

// External declarations for generated file data (after the type is defined)
extern const iso_file_data_t iso_files_data[];
extern const int iso_files_count;

// Static variables
static bool ramdisk_initialized = false;
static uint8_t ramdisk[RAMDISK_SIZE];

static bool init_ramdisk(void) {
    if (ramdisk_initialized) {
        return true;
    }
    
    terminal_writestring("FAT32: Initializing RAM disk...\n");
    
    // Clear RAM disk
    for (size_t i = 0; i < sizeof(ramdisk); i++) {
        ramdisk[i] = 0;
    }
    
    // Create a minimal FAT32 boot sector
    fat32_boot_sector_t* boot = (fat32_boot_sector_t*)ramdisk;
    
    // Jump instruction
    boot->jmp_boot[0] = 0xEB;
    boot->jmp_boot[1] = 0x58;
    boot->jmp_boot[2] = 0x90;
    
    // OEM name
    memcpy(boot->oem_name, "SYNCWIDE", 8);
    
    // BPB
    boot->bytes_per_sector = 512;
    boot->sectors_per_cluster = 8;
    boot->reserved_sectors = 32;
    boot->num_fats = 2;
    boot->root_entries = 0;
    boot->total_sectors_16 = 0;
    boot->media_type = 0xF8;
    boot->fat_size_16 = 0;
    boot->sectors_per_track = 63;
    boot->num_heads = 255;
    boot->hidden_sectors = 0;
    boot->total_sectors_32 = sizeof(ramdisk) / 512;
    
    // FAT32 specific
    boot->fat_size_32 = 128; // 128 sectors for FAT
    boot->ext_flags = 0;
    boot->fs_version = 0;
    boot->root_cluster = 2;
    boot->fs_info = 1;
    boot->backup_boot_sector = 6;
    boot->drive_number = 0x80;
    boot->boot_signature = 0x29;
    boot->volume_id = 0x12345678;
    memcpy(boot->volume_label, "SYNCWIDEOS ", 11);
    memcpy(boot->fs_type, "FAT32   ", 8);
    boot->signature = 0xAA55;
    
    // Create FSInfo sector
    fat32_fsinfo_t* fsinfo = (fat32_fsinfo_t*)(ramdisk + 512);
    fsinfo->lead_signature = FAT32_FSINFO_SIGNATURE1;
    fsinfo->struct_signature = FAT32_FSINFO_SIGNATURE2;
    fsinfo->free_count = 0xFFFFFFFF; // Unknown
    fsinfo->next_free = 3;
    fsinfo->trail_signature = FAT32_FSINFO_SIGNATURE3;
    
    // Initialize FAT (mark first few clusters as used)
    uint32_t* fat = (uint32_t*)(ramdisk + 32 * 512); // FAT starts at sector 32
    fat[0] = 0x0FFFFFF8; // Media descriptor
    fat[1] = 0x0FFFFFFF; // End of chain
    fat[2] = 0x0FFFFFFF; // Root directory (end of chain)
    
    // Get root directory location
    uint32_t root_sector = 32 + (2 * 128); // After FATs
    uint32_t root_cluster_sector = root_sector + (2 - 2) * 8; // Cluster 2
    fat32_dir_entry_t* root_entries = (fat32_dir_entry_t*)(ramdisk + root_cluster_sector * 512);
    
    // Load files from generated data
    if (iso_files_count > 0) {
        terminal_writestring("FAT32: Loading files from iso_files data...\n");
        
        uint32_t next_cluster = 3;
        int files_added = 0;
        
        for (int i = 0; i < iso_files_count && files_added < 16; i++) {
            const iso_file_data_t* file_data = &iso_files_data[i];
            
            // Create directory entry
            memcpy(root_entries[files_added].name, file_data->fat32_name, 11);
            root_entries[files_added].attributes = ATTR_ARCHIVE;
            root_entries[files_added].first_cluster_low = next_cluster;
            root_entries[files_added].first_cluster_high = 0;
            root_entries[files_added].file_size = file_data->size;
            
            // Mark cluster as end of chain
            fat[next_cluster] = 0x0FFFFFFF;
            
            // Write file content
            uint32_t file_sector = root_sector + (next_cluster - 2) * 8;
            char* file_content = (char*)(ramdisk + file_sector * 512);
            
            // Copy content (limit to cluster size)
            size_t max_size = 8 * 512; // 8 sectors per cluster
            size_t copy_size = file_data->size > max_size ? max_size : file_data->size;
            memcpy(file_content, file_data->content, copy_size);
            
            // Update file size if truncated
            if (copy_size < file_data->size) {
                root_entries[files_added].file_size = copy_size;
            }
            
            // Debug output
            terminal_writestring("FAT32: Added file: ");
            terminal_writestring(file_data->filename);
            terminal_writestring("\n");
            
            files_added++;
            next_cluster++;
        }
        
        terminal_writestring("FAT32: Successfully loaded files\n");
    } else {
        terminal_writestring("FAT32: No files found, creating default test file\n");
        
        // Fallback: create default test file
        memcpy(root_entries[0].name, "TEST    TXT", 11);
        root_entries[0].attributes = ATTR_ARCHIVE;
        root_entries[0].first_cluster_low = 3;
        root_entries[0].first_cluster_high = 0;
        root_entries[0].file_size = 30;
        
        fat[3] = 0x0FFFFFFF;
        
        uint32_t test_file_sector = root_sector + (3 - 2) * 8;
        char* test_content = (char*)(ramdisk + test_file_sector * 512);
        memcpy(test_content, "Hello from FAT32 filesystem!\n", 30);
    }
    
    ramdisk_initialized = true;
    terminal_writestring("FAT32: RAM disk initialized\n");
    return true;
}

// Public function to initialize FAT32
bool fat32_init(void) {
    return init_ramdisk();
}

// Public function to mount FAT32
bool fat32_mount(void) {
    if (!ramdisk_initialized) {
        if (!init_ramdisk()) {
            return false;
        }
    }
    
    terminal_writestring("FAT32: Filesystem mounted\n");
    return true;
}
