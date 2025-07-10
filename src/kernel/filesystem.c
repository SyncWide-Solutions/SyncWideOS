#include "../include/filesystem.h"
#include "../include/string.h"
#include "../include/disk.h"
#include "../include/vga.h"
#include "../include/memory.h"

// Storage device interface
extern void terminal_writestring(const char* data);
extern void terminal_clear(void);
extern void disk_init(void);
extern void memory_init(void);

// Global filesystem state
static fat32_fs_t g_fs;
static fs_file_handle_t g_file_handles[32];
static uint8_t g_cluster_buffer[SECTOR_SIZE * 8];
static fs_cwd_t g_cwd;

// Simple RAM disk for testing
static uint8_t ramdisk[1024 * 1024 * 4]; // 4MB RAM disk
static bool ramdisk_initialized = false;

// Helper function to convert cluster to sectors
static uint32_t cluster_to_sector(uint32_t cluster) {
    if (cluster < 2) return 0;
    return g_fs.data_start_sector + (cluster - 2) * g_fs.sectors_per_cluster;
}

// Storage interface implementation for RAM disk
bool storage_read_sectors(uint32_t sector, uint32_t count, void* buffer) {
    if (!ramdisk_initialized) {
        // Initialize simple RAM disk on first access
        for (size_t i = 0; i < sizeof(ramdisk); i++) {
            ramdisk[i] = 0;
        }
        
        // Create minimal FAT32 boot sector
        fat32_boot_sector_t* boot = (fat32_boot_sector_t*)ramdisk;
        boot->bytes_per_sector = 512;
        boot->sectors_per_cluster = 8;
        boot->reserved_sectors = 32;
        boot->num_fats = 2;
        boot->fat_size_32 = 64;
        boot->root_cluster = 2;
        boot->total_sectors_32 = sizeof(ramdisk) / 512;
        boot->signature = 0xAA55;
        
        // Initialize FAT
        uint32_t* fat = (uint32_t*)(ramdisk + 32 * 512);
        fat[0] = 0x0FFFFFF8;
        fat[1] = 0x0FFFFFFF;
        fat[2] = 0x0FFFFFFF; // Root directory
        
        // Create a simple test file in root directory
        uint32_t root_sector = 32 + (2 * 64); // After FATs
        uint32_t root_cluster_sector = root_sector + (2 - 2) * 8; // Cluster 2
        fat32_dir_entry_t* root_entries = (fat32_dir_entry_t*)(ramdisk + root_cluster_sector * 512);
        
        // Create test.txt file
        for (int i = 0; i < 11; i++) {
            root_entries[0].name[i] = ' ';
        }
        root_entries[0].name[0] = 'T';
        root_entries[0].name[1] = 'E';
        root_entries[0].name[2] = 'S';
        root_entries[0].name[3] = 'T';
        root_entries[0].name[8] = 'T';
        root_entries[0].name[9] = 'X';
        root_entries[0].name[10] = 'T';
        root_entries[0].attributes = ATTR_ARCHIVE;
        root_entries[0].first_cluster_low = 3;
        root_entries[0].first_cluster_high = 0;
        root_entries[0].file_size = 26;
        
        // Mark cluster 3 as end of chain
        fat[3] = 0x0FFFFFFF;
        
        // Write test file content
        uint32_t test_file_sector = root_sector + (3 - 2) * 8;
        char* test_content = (char*)(ramdisk + test_file_sector * 512);
        strcpy(test_content, "Hello from FAT32 system!\n");
        
        ramdisk_initialized = true;
        terminal_writestring("FAT32: RAM disk initialized with test file\n");
    }
    
    if (sector * 512 + count * 512 > sizeof(ramdisk)) {
        return false;
    }
    
    uint8_t* src = ramdisk + (sector * 512);
    uint8_t* dst = (uint8_t*)buffer;
    
    for (uint32_t i = 0; i < count * 512; i++) {
        dst[i] = src[i];
    }
    
    return true;
}

bool storage_write_sectors(uint32_t sector, uint32_t count, const void* buffer) {
    if (!ramdisk_initialized) {
        storage_read_sectors(0, 1, g_cluster_buffer); // Initialize
    }
    
    if (sector * 512 + count * 512 > sizeof(ramdisk)) {
        return false;
    }
    
    uint8_t* dst = ramdisk + (sector * 512);
    const uint8_t* src = (const uint8_t*)buffer;
    
    for (uint32_t i = 0; i < count * 512; i++) {
        dst[i] = src[i];
    }
    
    return true;
}

// Initialize filesystem
bool fs_init(void) {
    terminal_writestring("Initializing FAT32 filesystem...\n");
    
    // Clear filesystem state
    g_fs.mounted = false;
    for (int i = 0; i < 32; i++) {
        g_file_handles[i].in_use = false;
    }
    
    // Initialize current working directory
    g_cwd.cluster = 0;
    strcpy(g_cwd.path, "/");
    
    // Try to mount the filesystem
    return fs_mount();
}

// Mount FAT32 filesystem
bool fs_mount(void) {
    if (g_fs.mounted) {
        return true;
    }
    
    terminal_writestring("FAT32: Mounting filesystem...\n");
    
    // Read boot sector
    if (!storage_read_sectors(0, 1, &g_fs.boot_sector)) {
        terminal_writestring("FAT32: Failed to read boot sector\n");
        return false;
    }
    
    // Basic validation
    if (g_fs.boot_sector.bytes_per_sector != SECTOR_SIZE) {
        terminal_writestring("FAT32: Invalid sector size\n");
        return false;
    }
    
    // Calculate filesystem parameters
    g_fs.fat_start_sector = g_fs.boot_sector.reserved_sectors;
    g_fs.fat_size = g_fs.boot_sector.fat_size_32;
    g_fs.data_start_sector = g_fs.fat_start_sector + (g_fs.boot_sector.num_fats * g_fs.fat_size);
    g_fs.root_cluster = g_fs.boot_sector.root_cluster;
    g_fs.sectors_per_cluster = g_fs.boot_sector.sectors_per_cluster;
    g_fs.bytes_per_cluster = g_fs.sectors_per_cluster * SECTOR_SIZE;
    
    uint32_t data_sectors = g_fs.boot_sector.total_sectors_32 - g_fs.data_start_sector;
    g_fs.total_clusters = data_sectors / g_fs.sectors_per_cluster;
    
    // Initialize free cluster tracking
    g_fs.free_clusters = 0;
    g_fs.next_free_cluster = 3; // Start searching from cluster 3
    
    // Count free clusters
    for (uint32_t cluster = 2; cluster < g_fs.total_clusters + 2; cluster++) {
        if (fat32_read_fat_entry(cluster) == FAT32_FREE) {
            g_fs.free_clusters++;
        }
    }
    
    // Set current working directory to root
    g_cwd.cluster = g_fs.root_cluster;
    strcpy(g_cwd.path, "/");
    
    g_fs.mounted = true;
    terminal_writestring("FAT32: Filesystem mounted successfully\n");
    
    return true;
}

// Get current working directory
const char* fs_getcwd(void) {
    if (!g_fs.mounted) {
        return "/";
    }
    return g_cwd.path;
}

// Change directory
bool fs_chdir(const char* path) {
    if (!g_fs.mounted || !path) {
        return false;
    }
    
    // Handle special cases
    if (strcmp(path, "/") == 0) {
        g_cwd.cluster = g_fs.root_cluster;
        strcpy(g_cwd.path, "/");
        return true;
    }
    
    // For now, only support root directory
    // TODO: Implement full path resolution
    if (strcmp(path, "..") == 0 && strcmp(g_cwd.path, "/") != 0) {
        // Go to parent directory (for now, just root)
        g_cwd.cluster = g_fs.root_cluster;
        strcpy(g_cwd.path, "/");
        return true;
    }
    
    return false; // Directory change not supported yet
}

// Read FAT entry
uint32_t fat32_read_fat_entry(uint32_t cluster) {
    if (!g_fs.mounted || cluster < 2 || cluster >= g_fs.total_clusters + 2) {
        return FAT32_EOC;
    }
    
    // Calculate FAT sector and offset
    uint32_t fat_offset = cluster * 4; // 4 bytes per FAT32 entry
    uint32_t fat_sector = g_fs.fat_start_sector + (fat_offset / SECTOR_SIZE);
    uint32_t sector_offset = fat_offset % SECTOR_SIZE;
    
    // Read FAT sector
    uint8_t sector_buffer[SECTOR_SIZE];
    if (!storage_read_sectors(fat_sector, 1, sector_buffer)) {
        return FAT32_EOC;
    }
    
    // Extract FAT entry (mask off upper 4 bits)
    uint32_t fat_entry = *(uint32_t*)(sector_buffer + sector_offset) & 0x0FFFFFFF;
    return fat_entry;
}

// Write FAT entry
bool fat32_write_fat_entry(uint32_t cluster, uint32_t value) {
    if (!g_fs.mounted || cluster < 2 || cluster >= g_fs.total_clusters + 2) {
        return false;
    }
    
    // Calculate FAT sector and offset
    uint32_t fat_offset = cluster * 4; // 4 bytes per FAT32 entry
    uint32_t fat_sector = g_fs.fat_start_sector + (fat_offset / SECTOR_SIZE);
    uint32_t sector_offset = fat_offset % SECTOR_SIZE;
    
    // Read FAT sector
    uint8_t sector_buffer[SECTOR_SIZE];
    if (!storage_read_sectors(fat_sector, 1, sector_buffer)) {
        return false;
    }
    
    // Update FAT entry (preserve upper 4 bits)
    uint32_t* fat_entry = (uint32_t*)(sector_buffer + sector_offset);
    *fat_entry = (*fat_entry & 0xF0000000) | (value & 0x0FFFFFFF);
    
    // Write back to all FAT copies
    for (uint8_t fat_num = 0; fat_num < g_fs.boot_sector.num_fats; fat_num++) {
        uint32_t current_fat_sector = g_fs.fat_start_sector + (fat_num * g_fs.fat_size) + (fat_offset / SECTOR_SIZE);
        if (!storage_write_sectors(current_fat_sector, 1, sector_buffer)) {
            return false;
        }
    }
    
    return true;
}

// Allocate a free cluster
uint32_t fat32_allocate_cluster(void) {
    if (!g_fs.mounted || g_fs.free_clusters == 0) {
        return 0;
    }
    
    // Search for a free cluster starting from next_free_cluster
    for (uint32_t cluster = g_fs.next_free_cluster; cluster < g_fs.total_clusters + 2; cluster++) {
        if (fat32_read_fat_entry(cluster) == FAT32_FREE) {
            // Mark cluster as end of chain
            if (fat32_write_fat_entry(cluster, FAT32_EOC)) {
                g_fs.free_clusters--;
                g_fs.next_free_cluster = cluster + 1;
                return cluster;
            }
        }
    }
    
    // Search from the beginning if we didn't find one
    for (uint32_t cluster = 2; cluster < g_fs.next_free_cluster; cluster++) {
        if (fat32_read_fat_entry(cluster) == FAT32_FREE) {
            // Mark cluster as end of chain
            if (fat32_write_fat_entry(cluster, FAT32_EOC)) {
                g_fs.free_clusters--;
                g_fs.next_free_cluster = cluster + 1;
                return cluster;
            }
        }
    }
    
    return 0; // No free clusters found
}

// Free a cluster chain
bool fat32_free_cluster_chain(uint32_t start_cluster) {
    if (!g_fs.mounted || start_cluster < 2) {
        return false;
    }
    
    uint32_t current_cluster = start_cluster;
    
    while (current_cluster >= 2 && current_cluster < FAT32_EOC) {
        uint32_t next_cluster = fat32_read_fat_entry(current_cluster);
        
        // Mark current cluster as free
        if (!fat32_write_fat_entry(current_cluster, FAT32_FREE)) {
            return false;
        }
        
        g_fs.free_clusters++;
        
        // Update next_free_cluster hint
        if (current_cluster < g_fs.next_free_cluster) {
            g_fs.next_free_cluster = current_cluster;
        }
        
        current_cluster = next_cluster;
    }
    
    return true;
}

// Read a cluster
bool fat32_read_cluster(uint32_t cluster, void* buffer) {
    if (!g_fs.mounted || cluster < 2) {
        return false;
    }
    
    uint32_t sector = cluster_to_sector(cluster);
    return storage_read_sectors(sector, g_fs.sectors_per_cluster, buffer);
}

// Write a cluster
bool fat32_write_cluster(uint32_t cluster, const void* buffer) {
    if (!g_fs.mounted || cluster < 2) {
        return false;
    }
    
    uint32_t sector = cluster_to_sector(cluster);
    return storage_write_sectors(sector, g_fs.sectors_per_cluster, buffer);
}

// Convert 8.3 filename to normal filename
static void fat32_83_to_name(const uint8_t* fat_name, char* output) {
    int out_pos = 0;
    
    // Copy name part (8 characters)
    for (int i = 0; i < 8 && fat_name[i] != ' '; i++) {
        output[out_pos++] = fat_name[i];
        }
    
    // Add extension if present
    if (fat_name[8] != ' ') {
        output[out_pos++] = '.';
        for (int i = 8; i < 11 && fat_name[i] != ' '; i++) {
            output[out_pos++] = fat_name[i];
        }
    }
    
    output[out_pos] = '\0';
}

// Convert normal filename to 8.3 format
static void fat32_name_to_83(const char* name, uint8_t* fat_name) {
    // Initialize with spaces
    for (int i = 0; i < 11; i++) {
        fat_name[i] = ' ';
    }
    
    // Find extension
    const char* ext = NULL;
    for (int i = strlen(name) - 1; i >= 0; i--) {
        if (name[i] == '.') {
            ext = &name[i];
            break;
        }
    }
    
    int name_len = ext ? (ext - name) : strlen(name);
    int ext_len = ext ? strlen(ext + 1) : 0;
    
    // Copy name part (max 8 characters)
    for (int i = 0; i < name_len && i < 8; i++) {
        fat_name[i] = (name[i] >= 'a' && name[i] <= 'z') ? name[i] - 32 : name[i]; // Convert to uppercase
    }
    
    // Copy extension (max 3 characters)
    if (ext && ext_len > 0) {
        for (int i = 0; i < ext_len && i < 3; i++) {
            fat_name[8 + i] = (ext[1 + i] >= 'a' && ext[1 + i] <= 'z') ? ext[1 + i] - 32 : ext[1 + i];
        }
    }
}

// Find directory entry in a cluster
static fat32_dir_entry_t* find_dir_entry(uint32_t cluster, const char* name) {
    if (!fat32_read_cluster(cluster, g_cluster_buffer)) {
        return NULL;
    }
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)g_cluster_buffer;
    int entries_per_cluster = g_fs.bytes_per_cluster / sizeof(fat32_dir_entry_t);
    
    uint8_t fat_name[11];
    fat32_name_to_83(name, fat_name);
    
    for (int i = 0; i < entries_per_cluster; i++) {
        if (entries[i].name[0] == 0) {
            break; // End of directory
        }
        
        if (entries[i].name[0] == 0xE5) {
            continue; // Deleted entry
        }
        
        if (entries[i].attributes == ATTR_LONG_NAME) {
            continue; // Skip long filename entries for now
        }
        
        // Compare names
        bool match = true;
        for (int j = 0; j < 11; j++) {
            if (entries[i].name[j] != fat_name[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            return &entries[i];
        }
    }
    
    return NULL;
}

// Find free directory entry slot
static int find_free_dir_entry_slot(uint32_t cluster) {
    if (!fat32_read_cluster(cluster, g_cluster_buffer)) {
        return -1;
    }
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)g_cluster_buffer;
    int entries_per_cluster = g_fs.bytes_per_cluster / sizeof(fat32_dir_entry_t);
    
    for (int i = 0; i < entries_per_cluster; i++) {
        if (entries[i].name[0] == 0 || entries[i].name[0] == 0xE5) {
            return i; // Found free slot
        }
    }
    
    return -1; // No free slots
}

// Create directory entry
static bool create_dir_entry(uint32_t parent_cluster, const char* name, uint32_t first_cluster, uint32_t size, uint8_t attributes) {
    if (!fat32_read_cluster(parent_cluster, g_cluster_buffer)) {
        return false;
    }
    
    int slot = find_free_dir_entry_slot(parent_cluster);
    if (slot == -1) {
        return false; // No free slots
    }
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)g_cluster_buffer;
    fat32_dir_entry_t* entry = &entries[slot];
    
    // Convert filename to 8.3 format
    fat32_name_to_83(name, entry->name);
    
    // Set entry fields
    entry->attributes = attributes;
    entry->nt_reserved = 0;
    entry->creation_time_tenth = 0;
    entry->creation_time = 0;
    entry->creation_date = 0;
    entry->last_access_date = 0;
    entry->first_cluster_high = (first_cluster >> 16) & 0xFFFF;
    entry->write_time = 0;
    entry->write_date = 0;
    entry->first_cluster_low = first_cluster & 0xFFFF;
    entry->file_size = size;
    
    // Write back the cluster
    return fat32_write_cluster(parent_cluster, g_cluster_buffer);
}

// Open a file
fs_file_handle_t* fs_open(const char* path, const char* mode) {
    if (!g_fs.mounted || !path) {
        return NULL;
    }
    
    // Find free file handle
    fs_file_handle_t* handle = NULL;
    for (int i = 0; i < 32; i++) {
        if (!g_file_handles[i].in_use) {
            handle = &g_file_handles[i];
            break;
        }
    }
    
    if (!handle) {
        return NULL; // No free handles
    }
    
    // For now, only support files in current directory
    uint32_t search_cluster = (path[0] == '/') ? g_fs.root_cluster : g_cwd.cluster;
    const char* filename = (path[0] == '/') ? path + 1 : path;
    
    fat32_dir_entry_t* entry = find_dir_entry(search_cluster, filename);
    
    // Check if we're creating a new file
    if (!entry && mode && (mode[0] == 'w' || mode[0] == 'a')) {
        // Allocate a cluster for the new file
        uint32_t new_cluster = fat32_allocate_cluster();
        if (new_cluster == 0) {
            return NULL; // No free clusters
        }
        
        // Clear the cluster
        for (int i = 0; i < g_fs.bytes_per_cluster; i++) {
            g_cluster_buffer[i] = 0;
        }
        fat32_write_cluster(new_cluster, g_cluster_buffer);
        
        // Create directory entry
        if (!create_dir_entry(search_cluster, filename, new_cluster, 0, ATTR_ARCHIVE)) {
            fat32_free_cluster_chain(new_cluster);
            return NULL;
        }
        
        // Initialize handle for new file
        handle->in_use = true;
        handle->first_cluster = new_cluster;
        handle->current_cluster = new_cluster;
        handle->cluster_offset = 0;
        handle->file_size = 0;
        handle->position = 0;
        handle->attributes = ATTR_ARCHIVE;
        handle->is_directory = false;
        strcpy(handle->filename, filename);
        
        return handle;
    }
    
    if (!entry) {
        return NULL; // File not found
    }
    
    // Initialize handle for existing file
    handle->in_use = true;
    handle->first_cluster = (entry->first_cluster_high << 16) | entry->first_cluster_low;
    handle->current_cluster = handle->first_cluster;
    handle->cluster_offset = 0;
    handle->file_size = entry->file_size;
    handle->position = 0;
    handle->attributes = entry->attributes;
    handle->is_directory = (entry->attributes & ATTR_DIRECTORY) != 0;
    
    // Copy filename
    fat32_83_to_name(entry->name, handle->filename);
    
    // For append mode, seek to end
    if (mode && mode[0] == 'a') {
        fs_seek(handle, handle->file_size);
    }
    
    return handle;
}

// Close a file
void fs_close(fs_file_handle_t* handle) {
    if (handle && handle->in_use) {
        handle->in_use = false;
    }
}

// Read from a file
size_t fs_read(fs_file_handle_t* handle, void* buffer, size_t size) {
    if (!handle || !handle->in_use || !buffer || handle->is_directory) {
        return 0;
    }
    
    if (handle->position >= handle->file_size) {
        return 0; // EOF
    }
    
    // Limit read size to remaining file data
    if (handle->position + size > handle->file_size) {
        size = handle->file_size - handle->position;
    }
    
    size_t bytes_read = 0;
    uint8_t* output = (uint8_t*)buffer;
    
    while (bytes_read < size && handle->current_cluster >= 2 && handle->current_cluster < FAT32_EOC) {
        // Read current cluster if needed
        if (handle->cluster_offset == 0) {
            if (!fat32_read_cluster(handle->current_cluster, g_cluster_buffer)) {
                break;
            }
        }
        
        // Calculate how much to read from this cluster
        size_t cluster_remaining = g_fs.bytes_per_cluster - handle->cluster_offset;
        size_t to_read = (size - bytes_read < cluster_remaining) ? size - bytes_read : cluster_remaining;
        
        // Copy data from cluster buffer
        for (size_t i = 0; i < to_read; i++) {
            output[bytes_read + i] = g_cluster_buffer[handle->cluster_offset + i];
        }
        
        bytes_read += to_read;
        handle->position += to_read;
        handle->cluster_offset += to_read;
        
        // Move to next cluster if current one is exhausted
        if (handle->cluster_offset >= g_fs.bytes_per_cluster) {
            handle->current_cluster = fat32_read_fat_entry(handle->current_cluster);
            handle->cluster_offset = 0;
        }
    }
    
    return bytes_read;
}

// Update file size in directory entry
static bool update_file_size(const char* filename, uint32_t parent_cluster, uint32_t new_size) {
    if (!fat32_read_cluster(parent_cluster, g_cluster_buffer)) {
        return false;
    }
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)g_cluster_buffer;
    int entries_per_cluster = g_fs.bytes_per_cluster / sizeof(fat32_dir_entry_t);
    
    uint8_t fat_name[11];
    fat32_name_to_83(filename, fat_name);
    
    for (int i = 0; i < entries_per_cluster; i++) {
        if (entries[i].name[0] == 0) {
            break; // End of directory
        }
        
        if (entries[i].name[0] == 0xE5) {
            continue; // Deleted entry
        }
        
        if (entries[i].attributes == ATTR_LONG_NAME) {
            continue; // Skip long filename entries
        }
        
        // Compare names
        bool match = true;
        for (int j = 0; j < 11; j++) {
            if (entries[i].name[j] != fat_name[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            entries[i].file_size = new_size;
            return fat32_write_cluster(parent_cluster, g_cluster_buffer);
        }
    }
    
    return false;
}

// Write to a file
size_t fs_write(fs_file_handle_t* handle, const void* buffer, size_t size) {
    if (!handle || !handle->in_use || !buffer || handle->is_directory) {
        return 0;
    }
    
    size_t bytes_written = 0;
    const uint8_t* input = (const uint8_t*)buffer;
    
    while (bytes_written < size) {
        // Check if we need to allocate a new cluster
        if (handle->current_cluster < 2 || handle->current_cluster >= FAT32_EOC) {
            uint32_t new_cluster = fat32_allocate_cluster();
            if (new_cluster == 0) {
                break; // No more free clusters
            }
            
            // Clear the new cluster
            for (int i = 0; i < g_fs.bytes_per_cluster; i++) {
                g_cluster_buffer[i] = 0;
            }
            
            if (handle->first_cluster < 2) {
                // This is the first cluster for the file
                handle->first_cluster = new_cluster;
                handle->current_cluster = new_cluster;
                handle->cluster_offset = 0;
            } else {
                // Link the previous cluster to this new one
                // We need to find the last cluster in the chain
                uint32_t last_cluster = handle->first_cluster;
                while (true) {
                    uint32_t next = fat32_read_fat_entry(last_cluster);
                    if (next >= FAT32_EOC) {
                        break;
                    }
                    last_cluster = next;
                }
                
                // Link last cluster to new cluster
                fat32_write_fat_entry(last_cluster, new_cluster);
                handle->current_cluster = new_cluster;
                handle->cluster_offset = 0;
            }
        }
        
        // Read current cluster if we're at the beginning
        if (handle->cluster_offset == 0) {
            if (!fat32_read_cluster(handle->current_cluster, g_cluster_buffer)) {
                break;
            }
        }
        
        // Calculate how much to write to this cluster
        size_t cluster_remaining = g_fs.bytes_per_cluster - handle->cluster_offset;
        size_t to_write = (size - bytes_written < cluster_remaining) ? size - bytes_written : cluster_remaining;
        
        // Copy data to cluster buffer
        for (size_t i = 0; i < to_write; i++) {
            g_cluster_buffer[handle->cluster_offset + i] = input[bytes_written + i];
        }
        
        // Write cluster back to disk
        if (!fat32_write_cluster(handle->current_cluster, g_cluster_buffer)) {
            break;
        }
        
                bytes_written += to_write;
        handle->position += to_write;
        handle->cluster_offset += to_write;
        
        // Update file size if we've extended it
        if (handle->position > handle->file_size) {
            handle->file_size = handle->position;
        }
        
        // Move to next cluster if current one is full
        if (handle->cluster_offset >= g_fs.bytes_per_cluster) {
            uint32_t next_cluster = fat32_read_fat_entry(handle->current_cluster);
            if (next_cluster >= FAT32_EOC) {
                // Need to allocate a new cluster for continued writing
                handle->current_cluster = FAT32_EOC;
            } else {
                handle->current_cluster = next_cluster;
            }
            handle->cluster_offset = 0;
        }
    }
    
    // Update file size in directory entry
    uint32_t parent_cluster = g_cwd.cluster; // Assume current directory for now
    update_file_size(handle->filename, parent_cluster, handle->file_size);
    
    return bytes_written;
}

// Seek in a file
bool fs_seek(fs_file_handle_t* handle, uint32_t position) {
    if (!handle || !handle->in_use || handle->is_directory) {
        return false;
    }
    
    if (position > handle->file_size) {
        position = handle->file_size;
    }
    
    // Reset to beginning and advance
    handle->current_cluster = handle->first_cluster;
    handle->cluster_offset = 0;
    handle->position = 0;
    
    // Advance to target position
    while (handle->position < position && handle->current_cluster >= 2 && handle->current_cluster < FAT32_EOC) {
        uint32_t cluster_remaining = g_fs.bytes_per_cluster - handle->cluster_offset;
        uint32_t to_advance = (position - handle->position < cluster_remaining) ? 
                             position - handle->position : cluster_remaining;
        
        handle->position += to_advance;
        handle->cluster_offset += to_advance;
        
        if (handle->cluster_offset >= g_fs.bytes_per_cluster) {
            handle->current_cluster = fat32_read_fat_entry(handle->current_cluster);
            handle->cluster_offset = 0;
        }
    }
    
    return true;
}

// Get current position in file
uint32_t fs_tell(fs_file_handle_t* handle) {
    if (!handle || !handle->in_use) {
        return 0;
    }
    return handle->position;
}

// Create directory
bool fs_mkdir(const char* path) {
    if (!g_fs.mounted || !path || !*path) {
        return false;
    }
    
    // Skip leading spaces
    while (*path == ' ') path++;
    
    if (!*path) {
        return false;
    }
    
    // Check if directory already exists
    if (fs_exists(path)) {
        return false;
    }
    
    // Allocate cluster for new directory
    uint32_t new_cluster = fat32_allocate_cluster();
    if (new_cluster == 0) {
        return false;
    }
    
    // Clear the cluster
    for (int i = 0; i < g_fs.bytes_per_cluster; i++) {
        g_cluster_buffer[i] = 0;
    }
    
    // Create . and .. entries
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)g_cluster_buffer;
    
    // Create "." entry (current directory)
    for (int i = 0; i < 11; i++) {
        entries[0].name[i] = ' ';
    }
    entries[0].name[0] = '.';
    entries[0].attributes = ATTR_DIRECTORY;
    entries[0].first_cluster_high = (new_cluster >> 16) & 0xFFFF;
    entries[0].first_cluster_low = new_cluster & 0xFFFF;
    entries[0].file_size = 0;
    
    // Create ".." entry (parent directory)
    for (int i = 0; i < 11; i++) {
        entries[1].name[i] = ' ';
    }
    entries[1].name[0] = '.';
    entries[1].name[1] = '.';
    entries[1].attributes = ATTR_DIRECTORY;
    entries[1].first_cluster_high = (g_cwd.cluster >> 16) & 0xFFFF;
    entries[1].first_cluster_low = g_cwd.cluster & 0xFFFF;
    entries[1].file_size = 0;
    
    // Write the directory cluster
    if (!fat32_write_cluster(new_cluster, g_cluster_buffer)) {
        fat32_free_cluster_chain(new_cluster);
        return false;
    }
    
    // Create directory entry in parent directory
    if (!create_dir_entry(g_cwd.cluster, path, new_cluster, 0, ATTR_DIRECTORY)) {
        fat32_free_cluster_chain(new_cluster);
        return false;
    }
    
    return true;
}

// Delete file
bool fs_delete(const char* path) {
    if (!g_fs.mounted || !path || !*path) {
        return false;
    }
    
    // Find the file
    uint32_t search_cluster = (path[0] == '/') ? g_fs.root_cluster : g_cwd.cluster;
    const char* filename = (path[0] == '/') ? path + 1 : path;
    
    if (!fat32_read_cluster(search_cluster, g_cluster_buffer)) {
        return false;
    }
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)g_cluster_buffer;
    int entries_per_cluster = g_fs.bytes_per_cluster / sizeof(fat32_dir_entry_t);
    
    uint8_t fat_name[11];
    fat32_name_to_83(filename, fat_name);
    
    for (int i = 0; i < entries_per_cluster; i++) {
        if (entries[i].name[0] == 0) {
            break; // End of directory
        }
        
        if (entries[i].name[0] == 0xE5) {
            continue; // Deleted entry
        }
        
        if (entries[i].attributes == ATTR_LONG_NAME) {
            continue; // Skip long filename entries
        }
        
        // Compare names
        bool match = true;
        for (int j = 0; j < 11; j++) {
            if (entries[i].name[j] != fat_name[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            // Don't delete directories
            if (entries[i].attributes & ATTR_DIRECTORY) {
                return false;
            }
            
            // Free the cluster chain
            uint32_t first_cluster = (entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            if (first_cluster >= 2) {
                fat32_free_cluster_chain(first_cluster);
            }
            
            // Mark directory entry as deleted
            entries[i].name[0] = 0xE5;
            
            // Write back the directory cluster
            return fat32_write_cluster(search_cluster, g_cluster_buffer);
        }
    }
    
    return false; // File not found
}

// Remove directory
bool fs_rmdir(const char* path) {
    if (!g_fs.mounted || !path || !*path) {
        return false;
    }
    
    // Find the directory
    uint32_t search_cluster = (path[0] == '/') ? g_fs.root_cluster : g_cwd.cluster;
    const char* dirname = (path[0] == '/') ? path + 1 : path;
    
    if (!fat32_read_cluster(search_cluster, g_cluster_buffer)) {
        return false;
    }
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)g_cluster_buffer;
    int entries_per_cluster = g_fs.bytes_per_cluster / sizeof(fat32_dir_entry_t);
    
    uint8_t fat_name[11];
    fat32_name_to_83(dirname, fat_name);
    
    for (int i = 0; i < entries_per_cluster; i++) {
        if (entries[i].name[0] == 0) {
            break; // End of directory
        }
        
        if (entries[i].name[0] == 0xE5) {
            continue; // Deleted entry
        }
        
        if (entries[i].attributes == ATTR_LONG_NAME) {
            continue; // Skip long filename entries
        }
        
        // Compare names
        bool match = true;
        for (int j = 0; j < 11; j++) {
            if (entries[i].name[j] != fat_name[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            // Only delete directories
            if (!(entries[i].attributes & ATTR_DIRECTORY)) {
                return false;
            }
            
            // Check if directory is empty (only . and .. entries)
            uint32_t dir_cluster = (entries[i].first_cluster_high << 16) | entries[i].first_cluster_low;
            if (!fat32_read_cluster(dir_cluster, g_cluster_buffer)) {
                return false;
            }
            
            fat32_dir_entry_t* dir_entries = (fat32_dir_entry_t*)g_cluster_buffer;
            for (int j = 2; j < entries_per_cluster; j++) { // Skip . and ..
                if (dir_entries[j].name[0] != 0 && dir_entries[j].name[0] != 0xE5) {
                    return false; // Directory not empty
                }
            }
            
            // Free the directory cluster
            if (dir_cluster >= 2) {
                fat32_free_cluster_chain(dir_cluster);
            }
            
            // Read parent directory again (buffer was overwritten)
            if (!fat32_read_cluster(search_cluster, g_cluster_buffer)) {
                return false;
            }
            entries = (fat32_dir_entry_t*)g_cluster_buffer;
            
            // Mark directory entry as deleted
            entries[i].name[0] = 0xE5;
            
            // Write back the parent directory cluster
            return fat32_write_cluster(search_cluster, g_cluster_buffer);
        }
    }
    
    return false; // Directory not found
}

// Rename file or directory
bool fs_rename(const char* old_path, const char* new_path) {
    if (!g_fs.mounted || !old_path || !new_path || !*old_path || !*new_path) {
        return false;
    }
    
    // Check if new name already exists
    if (fs_exists(new_path)) {
        return false;
    }
    
    // Find the old file
    uint32_t search_cluster = (old_path[0] == '/') ? g_fs.root_cluster : g_cwd.cluster;
    const char* old_filename = (old_path[0] == '/') ? old_path + 1 : old_path;
    const char* new_filename = (new_path[0] == '/') ? new_path + 1 : new_path;
    
    if (!fat32_read_cluster(search_cluster, g_cluster_buffer)) {
        return false;
    }
    
    fat32_dir_entry_t* entries = (fat32_dir_entry_t*)g_cluster_buffer;
    int entries_per_cluster = g_fs.bytes_per_cluster / sizeof(fat32_dir_entry_t);
    
    uint8_t old_fat_name[11];
    fat32_name_to_83(old_filename, old_fat_name);
    
    for (int i = 0; i < entries_per_cluster; i++) {
        if (entries[i].name[0] == 0) {
            break; // End of directory
        }
        
        if (entries[i].name[0] == 0xE5) {
            continue; // Deleted entry
        }
        
        if (entries[i].attributes == ATTR_LONG_NAME) {
            continue; // Skip long filename entries
        }
        
        // Compare names
        bool match = true;
        for (int j = 0; j < 11; j++) {
            if (entries[i].name[j] != old_fat_name[j]) {
                match = false;
                break;
            }
        }
        
        if (match) {
            // Update the filename
            fat32_name_to_83(new_filename, entries[i].name);
            
            // Write back the directory cluster
            return fat32_write_cluster(search_cluster, g_cluster_buffer);
        }
    }
    
    return false; // File not found
}

// Check if filesystem is mounted
bool fs_is_mounted(void) {
    return g_fs.mounted;
}

// Get free space
uint32_t fs_get_free_space(void) {
    return g_fs.free_clusters * g_fs.bytes_per_cluster;
}

// Get total space
uint32_t fs_get_total_space(void) {
    return g_fs.total_clusters * g_fs.bytes_per_cluster;
}

// List directory contents
bool fs_list_directory(const char* path, void (*callback)(const char* name, bool is_dir, uint32_t size)) {
    if (!g_fs.mounted || !callback) {
        return false;
    }
    
    // Determine which directory to list
    uint32_t cluster;
    if (!path || strcmp(path, "") == 0 || strcmp(path, ".") == 0) {
        cluster = g_cwd.cluster;
    } else if (strcmp(path, "/") == 0) {
        cluster = g_fs.root_cluster;
    } else {
        // For now, only support root and current directory
        return false;
    }
    
    while (cluster >= 2 && cluster < FAT32_EOC) {
        if (!fat32_read_cluster(cluster, g_cluster_buffer)) {
            return false;
        }
        
        fat32_dir_entry_t* entries = (fat32_dir_entry_t*)g_cluster_buffer;
        int entries_per_cluster = g_fs.bytes_per_cluster / sizeof(fat32_dir_entry_t);
        
        for (int i = 0; i < entries_per_cluster; i++) {
            if (entries[i].name[0] == 0) {
                return true; // End of directory
            }
            
            if (entries[i].name[0] == 0xE5) {
                continue; // Deleted entry
            }
            
            if (entries[i].attributes == ATTR_LONG_NAME) {
                continue; // Skip long filename entries
            }
            
            if (entries[i].attributes & ATTR_VOLUME_ID) {
                continue; // Skip volume label
            }
            
                        // Skip . and .. entries in listing
            if (entries[i].name[0] == '.' && (entries[i].name[1] == ' ' ||
                (entries[i].name[1] == '.' && entries[i].name[2] == ' '))) {
                continue;
            }
            
            // Convert filename
            char filename[13];
            fat32_83_to_name(entries[i].name, filename);
            
            bool is_dir = (entries[i].attributes & ATTR_DIRECTORY) != 0;
            callback(filename, is_dir, entries[i].file_size);
        }
        
        cluster = fat32_read_fat_entry(cluster);
    }
    
    return true;
}

// Check if file exists
bool fs_exists(const char* path) {
    if (!g_fs.mounted || !path) {
        return false;
    }
    
    uint32_t search_cluster = (path[0] == '/') ? g_fs.root_cluster : g_cwd.cluster;
    const char* filename = (path[0] == '/') ? path + 1 : path;
    
    fat32_dir_entry_t* entry = find_dir_entry(search_cluster, filename);
    return entry != NULL;
}

// Get file size
uint32_t fs_get_file_size(const char* path) {
    if (!g_fs.mounted || !path) {
        return 0;
    }
    
    uint32_t search_cluster = (path[0] == '/') ? g_fs.root_cluster : g_cwd.cluster;
    const char* filename = (path[0] == '/') ? path + 1 : path;
    
    fat32_dir_entry_t* entry = find_dir_entry(search_cluster, filename);
    return entry ? entry->file_size : 0;
}

// Unmount filesystem
void fs_unmount(void) {
    if (g_fs.mounted) {
        // Close all open files
        for (int i = 0; i < 32; i++) {
            if (g_file_handles[i].in_use) {
                fs_close(&g_file_handles[i]);
            }
        }
        
        g_fs.mounted = false;
        terminal_writestring("FAT32: Filesystem unmounted\n");
    }
}

// Legacy filesystem interface implementation
static fs_node_t fs_nodes[FS_MAX_FILES];
static size_t fs_node_count = 0;
static fs_node_t* fs_root = NULL;
static fs_node_t* fs_cwd = NULL;

fs_node_t* fs_get_root(void) {
    if (!fs_root) {
        // Initialize legacy filesystem if not already done
        fs_root = &fs_nodes[fs_node_count++];
        strcpy(fs_root->name, "root");
        fs_root->type = FS_TYPE_DIRECTORY;
        fs_root->parent = NULL;
        fs_root->child_count = 0;
        fs_cwd = fs_root;
        
        // Create some initial directories and files
        fs_create_directory(fs_root, "bin");
        fs_create_directory(fs_root, "home");
        fs_create_directory(fs_root, "etc");
        
        // Create a welcome file
        fs_node_t* welcome = fs_create_file(fs_root, "welcome.txt");
        if (welcome) {
            fs_write_file(welcome, "Welcome to SyncWideOS Filesystem!\nType 'help' for available commands.");
        }
        
        // Create sample images
        fs_create_sample_images();
    }
    return fs_root;
}

fs_node_t* fs_get_cwd(void) {
    if (!fs_cwd) {
        fs_get_root(); // Initialize if needed
    }
    return fs_cwd;
}

void fs_set_cwd(fs_node_t* dir) {
    if (dir && dir->type == FS_TYPE_DIRECTORY) {
        fs_cwd = dir;
    }
}

fs_node_t* fs_create_file(fs_node_t* dir, const char* name) {
    if (!dir || dir->type != FS_TYPE_DIRECTORY || dir->child_count >= FS_MAX_CHILDREN || fs_node_count >= FS_MAX_FILES) {
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
    file->child_count = 0;
    file->content[0] = '\0';
    
    // Add to parent directory
    dir->children[dir->child_count++] = file;
    
    return file;
}

fs_node_t* fs_create_directory(fs_node_t* dir, const char* name) {
    if (!dir || dir->type != FS_TYPE_DIRECTORY || dir->child_count >= FS_MAX_CHILDREN || fs_node_count >= FS_MAX_FILES) {
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
        current = fs_get_root();
        path++; // Skip the leading '/'
    } else if (path[0] == '~') {
        current = fs_get_root();
        path++; // Skip the '~'
        if (path[0] == '/') {
            path++; // Skip the '/' after '~'
        }
    } else {
        current = fs_get_cwd();
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
    if (node == fs_get_root()) {
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
    while (current && current != fs_get_root()) {
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

void fs_create_sample_images(void) {
    fs_node_t* root = fs_get_root();
    
    // Create a simple gradient image
    fs_node_t* gradient = fs_create_file(root, "gradient.img");
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
    fs_node_t* rainbow = fs_create_file(root, "rainbow.img");
    if (rainbow) {
        // Each row uses a different color from the VGA palette
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
    fs_node_t* logo = fs_create_file(root, "logo.bmp");
    if (logo) {
        // Create a simple logo with different colors
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
    fs_node_t* checkerboard = fs_create_file(root, "checker.pic");
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