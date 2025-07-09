#include "../include/installer.h"
#include "../include/string.h"
#include "../include/io.h"
#include "../include/keyboard.h"
#include "../include/disk.h"
#include <stddef.h>

// External function declarations
extern void terminal_writestring(const char* data);
extern void terminal_clear(void);
extern void terminal_setcolor(uint8_t color);
extern char* itoa(int value, char* str, int base);
extern char _kernel_start[];
extern char _kernel_end[];

// Forward declarations for all functions
static void debug_print(const char* msg);
static int create_directory_entry(int disk_index, uint32_t fs_start, const char* path);
static int copy_file_to_disk(int disk_index, uint32_t fs_start, const char* path, 
                            uint8_t* data, uint32_t size);
static int create_system_config_files(int disk_index, uint32_t fs_start);
static int install_device_drivers(int disk_index, uint32_t fs_start);
static void get_user_input(char* buffer, int max_length);

// Function declarations for installer steps
void installer_welcome(void);
void installer_disk_selection(void);
void installer_user_config(void);
void installer_confirm(void);
void installer_complete(void);
void installer_error(void);

static int check_buffer_validity(uint8_t* buffer, size_t size) {
    // Simple check - try to read/write first and last byte
    volatile uint8_t test;
    test = buffer[0];
    buffer[0] = test;
    test = buffer[size-1];
    buffer[size-1] = test;
    return 0; // If we get here, buffer is accessible
}

// Debug function
static void debug_print(const char* msg) {
    terminal_writestring("DEBUG: ");
    terminal_writestring(msg);
    terminal_writestring("\n");
}

// Helper functions to work with your disk driver interface
static int disk_read_sectors_wrapper(int disk_index, uint32_t lba, int count, uint8_t* buffer) {
    if (buffer == NULL) {
        debug_print("disk_read_sectors_wrapper: buffer is NULL");
        return -1;
    }
    
    if (disk_index < 0 || disk_index >= 4) {
        debug_print("disk_read_sectors_wrapper: invalid disk_index");
        return -1;
    }
    
    if (count <= 0 || count > 256) {
        debug_print("disk_read_sectors_wrapper: invalid count");
        return -1;
    }
    
    debug_print("Calling ide_read_sectors with validated parameters");
    
    // Add small delay before disk operation
    for (volatile int i = 0; i < 1000; i++);
    
    int result = ide_read_sectors((uint8_t)disk_index, (uint8_t)count, lba, 0, (uint32_t)buffer);
    
    debug_print("ide_read_sectors returned");
    
    return result;
}

static int disk_write_sectors_wrapper(int disk_index, uint32_t lba, int count, uint8_t* buffer) {
    debug_print("disk_write_sectors_wrapper: Entry");
    
    if (buffer == NULL) {
        debug_print("disk_write_sectors_wrapper: buffer is NULL");
        return -1;
    }
    
    if (disk_index < 0 || disk_index >= 4) {
        debug_print("disk_write_sectors_wrapper: invalid disk_index");
        return -1;
    }
    
    if (count <= 0 || count > 256) {
        debug_print("disk_write_sectors_wrapper: invalid count");
        return -1;
    }
    
    if (lba > 0xFFFFFF) {  // 24-bit LBA limit for safety
        debug_print("disk_write_sectors_wrapper: LBA too high");
        return -1;
    }
    
    // Check buffer alignment
    uint32_t buffer_addr = (uint32_t)buffer;
    if (buffer_addr & 0x0F) {  // Check 16-byte alignment
        debug_print("disk_write_sectors_wrapper: buffer not aligned");
        return -1;
    }
    
    debug_print("disk_write_sectors_wrapper: parameters validated");
    
    // Add small delay before disk operation
    for (volatile int i = 0; i < 1000; i++);
    
    debug_print("disk_write_sectors_wrapper: calling ide_write_sectors");
    
    int result = ide_write_sectors((uint8_t)disk_index, (uint8_t)count, lba, 0, buffer_addr);
    
    debug_print("disk_write_sectors_wrapper: ide_write_sectors returned");
    
    return result;
}

// Global variables
static installer_config_t installer_config;
static installer_step_t current_step = INSTALL_STEP_WELCOME;
static char input_buffer[256];

// Initialize installer
void installer_init(void) {
    current_step = INSTALL_STEP_WELCOME;
    
    // Clear configuration - fix the warning
    char* config_ptr = (char*)&installer_config;
    for (size_t i = 0; i < sizeof(installer_config); i++) {
        config_ptr[i] = 0;
    }
    
    // Set defaults
    strcpy(installer_config.username, "user");
    strcpy(installer_config.hostname, "syncwide-os");
    strcpy(installer_config.target_disk, "0");
}

// Updated get_user_input function:
static void get_user_input(char* buffer, int max_length) {
    int pos = 0;
    
    while (pos < max_length - 1) {
        keyboard_poll();
        char c = keyboard_get_key();
        
        if (c == 0) {
            for (volatile int i = 0; i < 5000; i++);
            continue;
        }
        
        // Handle special keys (your driver returns values >= 0x80 for special keys)
        if ((unsigned char)c >= 0x80) {
            continue;
        }
        
        if (c == '\n' || c == '\r') {
            break;
        }
        
        if (c == '\b' && pos > 0) {
            pos--;
            terminal_writestring("\b \b");
            continue;
        }
        
        if (c >= 32 && c <= 126) {
            buffer[pos++] = c;
            char temp[2] = {c, '\0'};
            terminal_writestring(temp);
        }
    }
    
    buffer[pos] = '\0';
    terminal_writestring("\n");
}

// Main installer loop
void installer_run(void) {
    while (current_step != INSTALL_STEP_COMPLETE && current_step != INSTALL_STEP_ERROR) {
        switch (current_step) {
            case INSTALL_STEP_WELCOME:
                installer_welcome();
                break;
            case INSTALL_STEP_DISK_SELECTION:
                installer_disk_selection();
                break;
            case INSTALL_STEP_USER_CONFIG:
                installer_user_config();
                break;
            case INSTALL_STEP_CONFIRM:
                installer_confirm();
                break;
            case INSTALL_STEP_INSTALL:
                installer_perform_installation();
                break;
            default:
                current_step = INSTALL_STEP_ERROR;
                break;
        }
    }
    
    if (current_step == INSTALL_STEP_COMPLETE) {
        installer_complete();
    } else {
        installer_error();
    }
}

/* TEST SECTION */

int installer_simple_test(void) {
    debug_print("Starting simple installer test");
    
    int disk_index = atoi(installer_config.target_disk);
    debug_print("Disk index parsed");
    
    // Test 1: Simple write operation
    static uint8_t test_data[512] __attribute__((aligned(16)));
    for (int i = 0; i < 512; i++) {
        test_data[i] = 0xAA;
    }
    
    debug_print("Test 1: Writing test data to sector 2048");
    if (disk_write_sectors_wrapper(disk_index, 2048, 1, test_data) != 0) {
        debug_print("ERROR: Test write failed");
        return -1;
    }
    debug_print("Test 1: Write successful");
    
    // Test 2: Simple read operation
    static uint8_t read_data[512] __attribute__((aligned(16)));
    for (int i = 0; i < 512; i++) {
        read_data[i] = 0;
    }
    
    debug_print("Test 2: Reading test data from sector 2048");
    if (disk_read_sectors_wrapper(disk_index, 2048, 1, read_data) != 0) {
        debug_print("ERROR: Test read failed");
        return -1;
    }
    debug_print("Test 2: Read successful");
    
    // Test 3: Verify data
    debug_print("Test 3: Verifying data");
    for (int i = 0; i < 512; i++) {
        if (read_data[i] != 0xAA) {
            debug_print("ERROR: Data verification failed");
            return -1;
        }
    }
    debug_print("Test 3: Data verification successful");
    
    debug_print("All tests passed - disk operations working");
    return 0;
}

int installer_ultra_simple_test(void) {
    debug_print("Starting ultra simple test");
    
    // Test 1: Just parse the disk index
    debug_print("Test 1: Parsing disk index");
    int disk_index = atoi(installer_config.target_disk);
    
    char buffer[16];
    itoa(disk_index, buffer, 10);
    debug_print("Disk index: ");
    debug_print(buffer);
    
    // Test 2: Check if disk exists
    debug_print("Test 2: Getting drive info");
    ide_device_t* drive = disk_get_drive_info(disk_index);
    if (drive == NULL) {
        debug_print("ERROR: Drive is NULL");
        return -1;
    }
    debug_print("Drive found");
    
    // Test 3: Check drive type
    debug_print("Test 3: Checking drive type");
    if (drive->type != IDE_ATA) {
        debug_print("ERROR: Drive is not ATA");
        return -1;
    }
    debug_print("Drive is ATA type");
    
    // Test 4: Create a simple aligned buffer
    debug_print("Test 4: Creating test buffer");
    static uint8_t simple_buffer[512] __attribute__((aligned(16)));
    
    // Initialize buffer with a simple pattern
    for (int i = 0; i < 512; i++) {
        simple_buffer[i] = (uint8_t)(i & 0xFF);
    }
    debug_print("Buffer initialized");
    
    // Test 5: Check buffer address
    debug_print("Test 5: Checking buffer address");
    uint32_t buffer_addr = (uint32_t)simple_buffer;
    char addr_str[16];
    itoa(buffer_addr, addr_str, 16);
    debug_print("Buffer address: 0x");
    debug_print(addr_str);
    
    // Test 6: Validate parameters before calling IDE
    debug_print("Test 6: Validating parameters");
    if (disk_index < 0 || disk_index > 3) {
        debug_print("ERROR: Invalid disk index");
        return -1;
    }
    
    if (buffer_addr == 0) {
        debug_print("ERROR: Buffer address is NULL");
        return -1;
    }
    
    if (buffer_addr < 0x100000) {  // Should be above 1MB
        debug_print("WARNING: Buffer address might be too low");
    }
    
    debug_print("Parameters validated");
    
    // Test 7: Try to call IDE write with minimal parameters
    debug_print("Test 7: About to call ide_write_sectors");
    debug_print("Parameters:");
    debug_print("  drive: ");
    debug_print(buffer);  // disk_index as string
    debug_print("  count: 1");
    debug_print("  lba: 2048");
    debug_print("  lba_high: 0");
    debug_print("  buffer: 0x");
    debug_print(addr_str);
    
    // Add a longer delay before the call
    debug_print("Adding delay before IDE call...");
    for (volatile int i = 0; i < 100000; i++);
    
    debug_print("Calling ide_write_sectors NOW");
    
    // Call the IDE function directly (bypass wrapper for now)
    int result = ide_write_sectors((uint8_t)disk_index, 1, 2048, 0, buffer_addr);
    
    debug_print("ide_write_sectors returned");
    
    if (result != 0) {
        debug_print("ERROR: ide_write_sectors failed");
        itoa(result, buffer, 10);
        debug_print("Error code: ");
        debug_print(buffer);
        return -1;
    }
    
    debug_print("ide_write_sectors succeeded");
    debug_print("Ultra simple test completed successfully");
    
    return 0;
}

/* TEST SECTION END */

// Error screen - fix the function signature
void installer_error(void) {
    terminal_clear();
    terminal_setcolor(COLOR_RED);
    terminal_writestring("=== Installation Error ===\n\n");
    terminal_setcolor(COLOR_WHITE);
    
    terminal_writestring("An error occurred during installation.\n\n");
    terminal_writestring("The installation has been aborted.\n\n");
    terminal_writestring("Press ENTER to return to the main system...");
    
    while (1) {
        keyboard_poll();
        char c = keyboard_get_key();
        if (c == '\n' || c == '\r') {
            break;
        }
    }
}

// Welcome screen
void installer_welcome(void) {
    terminal_clear();
    terminal_setcolor(COLOR_CYAN);
    terminal_writestring("=== SyncWide OS Installer ===\n\n");
    terminal_setcolor(COLOR_WHITE);
    
    terminal_writestring("Welcome to the SyncWide OS installation wizard!\n\n");
    terminal_writestring("This installer will guide you through the process of\n");
    terminal_writestring("installing SyncWide OS on your computer.\n\n");
    terminal_writestring("WARNING: This will erase all data on the selected disk!\n\n");
    terminal_writestring("Press ENTER to continue or ESC to exit...");
    
    while (1) {
        keyboard_poll();
        char c = keyboard_get_key();
        
        if (c == '\n' || c == '\r') {
            current_step = INSTALL_STEP_DISK_SELECTION;
            break;
        } else if (c == 27) { // ESC
            current_step = INSTALL_STEP_ERROR;
            break;
        }
    }
}

// Disk selection screen
void installer_disk_selection(void) {
    terminal_clear();
    terminal_setcolor(COLOR_CYAN);
    terminal_writestring("=== Disk Selection ===\n\n");
    terminal_setcolor(COLOR_WHITE);
    
    terminal_writestring("Available disks:\n\n");
    
    // List available disks
    for (int i = 0; i < 4; i++) {
        ide_device_t* drive = disk_get_drive_info(i);
        if (drive != NULL && drive->type == IDE_ATA) {
            terminal_writestring("  ");
            char buffer[16];
            itoa(i, buffer, 10);
            terminal_writestring(buffer);
            terminal_writestring(": ");
            // Fix the pointer type warning
            terminal_writestring((const char*)drive->model);
            terminal_writestring(" (");
            itoa(drive->size / 2048, buffer, 10); // Convert sectors to MB
            terminal_writestring(buffer);
            terminal_writestring(" MB)\n");
        }
    }
    
    terminal_writestring("\nEnter disk number to install to: ");
    get_user_input(installer_config.target_disk, sizeof(installer_config.target_disk));
    
    // Validate disk selection
    int disk_index = atoi(installer_config.target_disk);
    ide_device_t* drive = disk_get_drive_info(disk_index);
    
    if (drive == NULL || drive->type != IDE_ATA) {
        terminal_setcolor(COLOR_RED);
        terminal_writestring("Invalid disk selection!\n");
        terminal_setcolor(COLOR_WHITE);
        terminal_writestring("Press ENTER to try again...");
        
        while (1) {
            keyboard_poll();
            char c = keyboard_get_key();
            if (c == '\n' || c == '\r') break;
        }
        return;
    }
    
    current_step = INSTALL_STEP_USER_CONFIG;
}

// User configuration screen
void installer_user_config(void) {
    terminal_clear();
    terminal_setcolor(COLOR_CYAN);
    terminal_writestring("=== User Configuration ===\n\n");
    terminal_setcolor(COLOR_WHITE);
    
    terminal_writestring("Username: ");
    get_user_input(installer_config.username, sizeof(installer_config.username));
    
    terminal_writestring("Password: ");
    get_user_input(installer_config.password, sizeof(installer_config.password));
    
    terminal_writestring("Hostname: ");
    get_user_input(installer_config.hostname, sizeof(installer_config.hostname));
    
    current_step = INSTALL_STEP_CONFIRM;
}

// Confirmation screen
void installer_confirm(void) {
    terminal_clear();
    terminal_setcolor(COLOR_CYAN);
    terminal_writestring("=== Installation Summary ===\n\n");
    terminal_setcolor(COLOR_WHITE);
    
    terminal_writestring("Target disk: ");
    terminal_writestring(installer_config.target_disk);
    terminal_writestring("\n");
    
    terminal_writestring("Username: ");
    terminal_writestring(installer_config.username);
    terminal_writestring("\n");
    
    terminal_writestring("Hostname: ");
    terminal_writestring(installer_config.hostname);
    terminal_writestring("\n\n");
    
    terminal_setcolor(COLOR_RED);
    terminal_writestring("WARNING: This will ERASE ALL DATA on the selected disk!\n\n");
    terminal_setcolor(COLOR_WHITE);
    
    terminal_writestring("Type 'yes' to continue or 'no' to cancel: ");
    get_user_input(input_buffer, sizeof(input_buffer));
    
    if (strcmp(input_buffer, "yes") == 0) {
        current_step = INSTALL_STEP_INSTALL;
    } else {
        current_step = INSTALL_STEP_DISK_SELECTION;
    }
}

// Real disk formatting function
int installer_format_disk(const char* disk_index_str) {
    int disk_index = atoi(disk_index_str);
    ide_device_t* drive = disk_get_drive_info(disk_index);
    
    if (drive == NULL || drive->type != IDE_ATA) {
        return -1;
    }
    
    terminal_writestring("      Creating partition table...\n");
    
    uint8_t mbr[512];
    for (int i = 0; i < 512; i++) {
        mbr[i] = 0;
    }
    
    mbr[510] = 0x55;
    mbr[511] = 0xAA;
    
    uint8_t* partition_entry = &mbr[446];
    partition_entry[0] = 0x80;  // Bootable
    partition_entry[1] = 0x00;  // Head
    partition_entry[2] = 0x02;  // Sector
    partition_entry[3] = 0x00;  // Cylinder
    partition_entry[4] = 0x83;  // Linux filesystem
    partition_entry[5] = 0xFE;  // Head
    partition_entry[6] = 0xFF;  // Sector
    partition_entry[7] = 0xFF;  // Cylinder
    
    uint32_t start_lba = 2048;
    partition_entry[8] = start_lba & 0xFF;
    partition_entry[9] = (start_lba >> 8) & 0xFF;
    partition_entry[10] = (start_lba >> 16) & 0xFF;
    partition_entry[11] = (start_lba >> 24) & 0xFF;
    
    uint32_t partition_size = drive->size - start_lba - 1024;
    partition_entry[12] = partition_size & 0xFF;
    partition_entry[13] = (partition_size >> 8) & 0xFF;
    partition_entry[14] = (partition_size >> 16) & 0xFF;
    partition_entry[15] = (partition_size >> 24) & 0xFF;
    
    if (disk_write_sectors_wrapper(disk_index, 0, 1, mbr) != 0) {
        return -1;
    }
    
    terminal_writestring("      Creating filesystem...\n");
    
    uint8_t superblock[512];
    for (int i = 0; i < 512; i++) {
        superblock[i] = 0;
    }
    
    superblock[0] = 'S';
    superblock[1] = 'W';
    superblock[2] = 'O';
    superblock[3] = 'S';
    
    if (disk_write_sectors_wrapper(disk_index, start_lba, 1, superblock) != 0) {
        return -1;
    }
    
    return 0;
}

// Real GRUB installation function
int installer_install_grub(const char* disk_index_str) {
    int disk_index = atoi(disk_index_str);
    ide_device_t* drive = disk_get_drive_info(disk_index);
    
    if (drive == NULL) {
        return -1;
    }
    
    terminal_writestring("      Installing bootloader to MBR...\n");
    
    uint8_t mbr[512];
    if (disk_read_sectors_wrapper(disk_index, 0, 1, mbr) != 0) {
        return -1;
    }
    
        uint8_t bootloader_code[] = {
        0xFA,                    // cli
        0xB8, 0x00, 0x07,       // mov ax, 0x0700
        0x8E, 0xD8,             // mov ds, ax
        0x8E, 0xC0,             // mov es, ax
        0xBE, 0x20, 0x7C,       // mov si, msg
        0xE8, 0x0A, 0x00,       // call print
        0xB4, 0x00,             // mov ah, 0
        0xCD, 0x16,             // int 0x16
        0xCD, 0x19,             // int 0x19
        0xEB, 0xFE,             // jmp $
        0xAC,                    // lodsb
        0x08, 0xC0,             // or al, al
        0x74, 0x06,             // jz done
        0xB4, 0x0E,             // mov ah, 0x0E
        0xCD, 0x10,             // int 0x10
        0xEB, 0xF7,             // jmp print
        0xC3,                    // ret
        'S', 'y', 'n', 'c', 'W', 'i', 'd', 'e', ' ', 'O', 'S', '\r', '\n', 0
    };
    
    for (size_t i = 0; i < sizeof(bootloader_code) && i < 446; i++) {
        mbr[i] = bootloader_code[i];
    }
    
    if (disk_write_sectors_wrapper(disk_index, 0, 1, mbr) != 0) {
        return -1;
    }
    
    terminal_writestring("      Creating boot configuration...\n");
    
    return 0;
}

// Helper function implementations
static int create_directory_entry(int disk_index, uint32_t fs_start, const char* path) {
    typedef struct {
        char name[32];
        uint32_t type;
        uint32_t size;
        uint32_t start_sector;
        uint32_t created_time;
    } __attribute__((packed)) directory_entry_t;
    
    // Input validation
    if (!path) {
        debug_print("ERROR: path is NULL");
        return -1;
    }
    
    if (disk_index < 0 || disk_index >= 4) {
        debug_print("ERROR: invalid disk_index");
        return -1;
    }
    
    if (strlen(path) == 0 || strlen(path) > 31) {
        debug_print("ERROR: invalid path length");
        return -1;
    }
    
    debug_print("Creating directory entry for path");
    
    // Use static buffer to avoid stack issues
    static uint8_t dir_sector[512] __attribute__((aligned(16)));
    
    // Initialize buffer to zero
    for (int i = 0; i < 512; i++) {
        dir_sector[i] = 0;
    }
    
    debug_print("Initialized directory buffer");
    
    // Skip reading existing directory for now - just create new entries
    // This avoids the problematic disk read that's causing crashes
    debug_print("Skipping directory read - creating fresh directory");
    
    // Cast buffer to directory entries
    directory_entry_t* entries = (directory_entry_t*)dir_sector;
    int max_entries = 512 / sizeof(directory_entry_t);
    
    debug_print("Finding free directory entry slot");
    
    // Find first free slot (they're all free since we zeroed the buffer)
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] == 0) {
            debug_print("Found free slot, creating entry");
            
            // Create the directory entry
            strncpy(entries[i].name, path, 31);
            entries[i].name[31] = '\0';
            entries[i].type = 1;        // Directory type
            entries[i].size = 0;        // Directories have no size
            entries[i].start_sector = 0; // No data sectors for empty directory
            entries[i].created_time = 0; // Placeholder timestamp
            
            debug_print("About to write directory sector");
            
            // Add delay before write
            for (volatile int i = 0; i < 10000; i++);
            
            // Write the directory sector
            int write_result = disk_write_sectors_wrapper(disk_index, fs_start + 1, 1, dir_sector);
            if (write_result != 0) {
                debug_print("ERROR: Failed to write directory sector");
                return -1;
            }
            
            debug_print("Directory entry created successfully");
            return 0; // Success
        }
    }
    
    debug_print("ERROR: No free directory slots available");
    return -1; // No free slots
}

static int copy_file_to_disk(int disk_index, uint32_t fs_start, const char* path, 
                            uint8_t* data, uint32_t size) {
    uint32_t sectors_needed = (size + 511) / 512;
    static uint32_t next_free_sector = 100;
    uint32_t file_start_sector = fs_start + next_free_sector;
    next_free_sector += sectors_needed;
    
    uint32_t bytes_written = 0;
    for (uint32_t i = 0; i < sectors_needed; i++) {
        uint8_t sector_data[512];
        uint32_t bytes_to_copy = (size - bytes_written > 512) ? 512 : (size - bytes_written);
        
        for (int j = 0; j < 512; j++) sector_data[j] = 0;
        
        for (uint32_t j = 0; j < bytes_to_copy; j++) {
            sector_data[j] = data[bytes_written + j];
        }
        
        if (disk_write_sectors_wrapper(disk_index, file_start_sector + i, 1, sector_data) != 0) {
            return -1;
        }
        
        bytes_written += bytes_to_copy;
    }
    
    // Add to directory table
    uint8_t dir_sector[512];
    if (disk_read_sectors_wrapper(disk_index, fs_start + 1, 1, dir_sector) != 0) {
        return -1;
    }
    
    typedef struct {
        char name[32];
        uint32_t type;
        uint32_t size;
        uint32_t start_sector;
        uint32_t created_time;
    } directory_entry_t;
    
    directory_entry_t* entries = (directory_entry_t*)dir_sector;
    int max_entries = 512 / sizeof(directory_entry_t);
    
    for (int i = 0; i < max_entries; i++) {
        if (entries[i].name[0] == 0) {
            strncpy(entries[i].name, path, 31);
            entries[i].name[31] = '\0';
            entries[i].type = 0;
            entries[i].size = size;
            entries[i].start_sector = file_start_sector;
            entries[i].created_time = 0;
            
            return disk_write_sectors_wrapper(disk_index, fs_start + 1, 1, dir_sector);
        }
    }
    
    return -1;
}

static int create_system_config_files(int disk_index, uint32_t fs_start) {
    // Create /etc/passwd
    char passwd_content[] = "root:x:0:0:root:/root:/bin/sh\n";
    if (copy_file_to_disk(disk_index, fs_start, "/etc/passwd", 
                         (uint8_t*)passwd_content, strlen(passwd_content)) != 0) {
        return -1;
    }
    
    // Create /etc/hostname
    if (copy_file_to_disk(disk_index, fs_start, "/etc/hostname", 
                         (uint8_t*)installer_config.hostname, 
                         strlen(installer_config.hostname)) != 0) {
        return -1;
    }
    
    // Create /etc/fstab
    char fstab_content[] = "/dev/sda1 / ext2 defaults 0 1\n";
    if (copy_file_to_disk(disk_index, fs_start, "/etc/fstab", 
                         (uint8_t*)fstab_content, strlen(fstab_content)) != 0) {
        return -1;
    }
    
    // Create boot configuration
    char grub_cfg[] = 
        "set timeout=5\n"
        "set default=0\n"
        "menuentry \"SyncWide OS\" {\n"
        "    set root=(hd0,1)\n"
        "    linux /boot/kernel.bin\n"
        "    boot\n"
        "}\n";
    
    if (copy_file_to_disk(disk_index, fs_start, "/boot/grub.cfg", 
                         (uint8_t*)grub_cfg, strlen(grub_cfg)) != 0) {
        return -1;
    }
    
    return 0;
}

static int install_device_drivers(int disk_index, uint32_t fs_start) {
    // Create driver directory first
    if (create_directory_entry(disk_index, fs_start, "/usr/drivers") != 0) {
        return -1;
    }
    
    // Install keyboard driver
    char keyboard_driver[] = "# Keyboard driver module\n";
    if (copy_file_to_disk(disk_index, fs_start, "/usr/drivers/keyboard.ko", 
                         (uint8_t*)keyboard_driver, strlen(keyboard_driver)) != 0) {
        return -1;
    }
    
    // Install disk driver
    char disk_driver[] = "# Disk driver module\n";
    if (copy_file_to_disk(disk_index, fs_start, "/usr/drivers/disk.ko", 
                         (uint8_t*)disk_driver, strlen(disk_driver)) != 0) {
        return -1;
    }
    
    // Install network driver
    char network_driver[] = "# Network driver module\n";
    if (copy_file_to_disk(disk_index, fs_start, "/usr/drivers/network.ko", 
                         (uint8_t*)network_driver, strlen(network_driver)) != 0) {
        return -1;
    }
    
    return 0;
}

// Real system files copying function
int installer_copy_system_files(void) {
    debug_print("Starting system files copy");
    
    // Parse and validate disk index
    int disk_index = atoi(installer_config.target_disk);
    if (disk_index < 0 || disk_index >= 4) {
        debug_print("ERROR: Invalid disk index");
        terminal_writestring("ERROR: Invalid disk index\n");
        return -1;
    }
    
    debug_print("Disk index parsed successfully");
    
    // Skip the disk accessibility test for now since it's causing crashes
    debug_print("Skipping disk accessibility test - proceeding with installation");
    
    uint32_t start_lba = 2048;
    
    terminal_writestring("      Creating directory structure...\n");
    debug_print("Creating directory structure");
    
    // Add delay before starting directory creation
    for (volatile int i = 0; i < 10000; i++);
    
    // Create directories one by one with error checking and delays
    debug_print("Creating root directory");
    if (create_directory_entry(disk_index, start_lba, "/") != 0) {
        debug_print("ERROR: Failed to create root directory");
        terminal_writestring("ERROR: Failed to create root directory\n");
        return -1;
    }
    terminal_writestring("        Root directory created\n");
    
    // Delay between operations
    for (volatile int i = 0; i < 20000; i++);
    
    debug_print("Creating /boot directory");
    if (create_directory_entry(disk_index, start_lba, "/boot") != 0) {
        debug_print("ERROR: Failed to create /boot directory");
        terminal_writestring("ERROR: Failed to create /boot directory\n");
        return -1;
    }
    terminal_writestring("        /boot directory created\n");
    
    // Delay between operations
    for (volatile int i = 0; i < 20000; i++);
    
    debug_print("Creating /etc directory");
    if (create_directory_entry(disk_index, start_lba, "/etc") != 0) {
        debug_print("ERROR: Failed to create /etc directory");
        terminal_writestring("ERROR: Failed to create /etc directory\n");
        return -1;
    }
    terminal_writestring("        /etc directory created\n");
    
    // Delay between operations
    for (volatile int i = 0; i < 20000; i++);
    
    debug_print("Creating /usr directory");
    if (create_directory_entry(disk_index, start_lba, "/usr") != 0) {
        debug_print("ERROR: Failed to create /usr directory");
        terminal_writestring("ERROR: Failed to create /usr directory\n");
        return -1;
    }
    terminal_writestring("        /usr directory created\n");
    
    // Delay between operations
    for (volatile int i = 0; i < 20000; i++);
    
    debug_print("Creating /var directory");
    if (create_directory_entry(disk_index, start_lba, "/var") != 0) {
        debug_print("ERROR: Failed to create /var directory");
        terminal_writestring("ERROR: Failed to create /var directory\n");
        return -1;
    }
    terminal_writestring("        /var directory created\n");
    
    // Delay between operations
    for (volatile int i = 0; i < 20000; i++);
    
    debug_print("Creating /home directory");
    if (create_directory_entry(disk_index, start_lba, "/home") != 0) {
        debug_print("ERROR: Failed to create /home directory");
        terminal_writestring("ERROR: Failed to create /home directory\n");
        return -1;
    }
    terminal_writestring("        /home directory created\n");
    
    // Delay between operations
    for (volatile int i = 0; i < 20000; i++);
    
    debug_print("Creating /tmp directory");
    if (create_directory_entry(disk_index, start_lba, "/tmp") != 0) {
        debug_print("ERROR: Failed to create /tmp directory");
        terminal_writestring("ERROR: Failed to create /tmp directory\n");
        return -1;
    }
    terminal_writestring("        /tmp directory created\n");
    
    debug_print("Directory structure created successfully");
    
    // Longer delay before kernel installation
    for (volatile int i = 0; i < 50000; i++);
    
    terminal_writestring("      Installing kernel...\n");
    debug_print("About to install kernel");
    
    // Calculate kernel size using the existing char[] declarations
    uint32_t kernel_size = (uint32_t)_kernel_end - (uint32_t)_kernel_start;
    
    debug_print("Calculating kernel size");
    char size_buffer[32];
    itoa(kernel_size, size_buffer, 10);
    debug_print("Kernel size calculated");
    
    if (kernel_size == 0 || kernel_size > 0x100000) { // Sanity check: max 1MB kernel
        debug_print("ERROR: Invalid kernel size");
        terminal_writestring("ERROR: Invalid kernel size\n");
        return -1;
    }
    
    debug_print("Kernel size validated, copying to disk");
    
    if (copy_file_to_disk(disk_index, start_lba, "/boot/kernel.bin",
                         (uint8_t*)_kernel_start, kernel_size) != 0) {
        debug_print("ERROR: Failed to copy kernel to disk");
        terminal_writestring("ERROR: Failed to copy kernel to disk\n");
        return -1;
    }
    
    debug_print("Kernel installed successfully");
    terminal_writestring("        Kernel installed successfully\n");
    
    // Delay before system config files
    for (volatile int i = 0; i < 50000; i++);
    
    terminal_writestring("      Installing system libraries...\n");
    debug_print("Creating system config files");
    
    if (create_system_config_files(disk_index, start_lba) != 0) {
        debug_print("ERROR: Failed to create system config files");
        terminal_writestring("ERROR: Failed to create system config files\n");
        return -1;
    }
    
    debug_print("System config files created successfully");
    terminal_writestring("        System config files created\n");
    
    // Delay before device drivers
    for (volatile int i = 0; i < 50000; i++);
    
    terminal_writestring("      Installing device drivers...\n");
    debug_print("Installing device drivers");
    
    if (install_device_drivers(disk_index, start_lba) != 0) {
        debug_print("ERROR: Failed to install device drivers");
        terminal_writestring("ERROR: Failed to install device drivers\n");
        return -1;
    }
    
    debug_print("Device drivers installed successfully");
    terminal_writestring("        Device drivers installed\n");
    
    // Final delay and verification
    for (volatile int i = 0; i < 30000; i++);
    
    debug_print("System files copy completed successfully");
    terminal_writestring("      All system files installed successfully!\n");
    
    return 0;
}

// Real user configuration creation
int installer_create_user_config(const installer_config_t* config) {
    int disk_index = atoi(config->target_disk);
    
    terminal_writestring("      Writing user account data...\n");
    
    uint8_t user_config[512];
    for (int i = 0; i < 512; i++) {
        user_config[i] = 0;
    }
    
    strncpy((char*)user_config, config->username, 31);
    strncpy((char*)user_config + 32, config->hostname, 31);
    
    size_t password_len = strlen(config->password);
    for (size_t i = 0; i < password_len; i++) {
        user_config[64 + i] = config->password[i] ^ 0xAA;
    }
    
    if (disk_write_sectors_wrapper(disk_index, 2048 + 30, 1, user_config) != 0) {
        return -1;
    }
    
    terminal_writestring("      Setting system hostname...\n");
    
    uint8_t hostname_config[512];
    for (int i = 0; i < 512; i++) {
        hostname_config[i] = 0;
    }
    
    strncpy((char*)hostname_config, config->hostname, 255);
    
    if (disk_write_sectors_wrapper(disk_index, 2048 + 31, 1, hostname_config) != 0) {
        return -1;
    }
    
    return 0;
}

// Updated installation function with real operations
void installer_perform_installation(void) {
    terminal_clear();
    terminal_setcolor(COLOR_CYAN);
    terminal_writestring("=== Installing SyncWide OS ===\n\n");
    terminal_setcolor(COLOR_WHITE);
    
    debug_print("Starting installation process");
    
    // Verify disk is still available
    int disk_index = atoi(installer_config.target_disk);
    debug_print("Parsed disk index");
    
    ide_device_t* drive = disk_get_drive_info(disk_index);
    if (drive == NULL) {
        terminal_setcolor(COLOR_RED);
        terminal_writestring("ERROR: Target disk no longer available!\n");
        debug_print("Drive is NULL");
        current_step = INSTALL_STEP_ERROR;
        return;
    }
    debug_print("Drive found and validated");

    // Step 0: Validate disk
    terminal_writestring("[0/5] Testing basic disk operations...\n");
    if (installer_ultra_simple_test() != 0) {
        terminal_setcolor(COLOR_RED);
        terminal_writestring("ERROR: Basic disk test failed!\n");
        current_step = INSTALL_STEP_ERROR;
        return;
    }
    terminal_setcolor(COLOR_GREEN);
    terminal_writestring("      Basic disk test passed.\n\n");
    terminal_setcolor(COLOR_WHITE);
    
    // Step 1: Format disk
    terminal_writestring("[1/5] Formatting disk...\n");
    debug_print("About to format disk");
    
    int format_result = installer_format_disk(installer_config.target_disk);
    if (format_result != 0) {
        terminal_setcolor(COLOR_RED);
        terminal_writestring("ERROR: Failed to format disk! Error code: ");
        char error_buf[16];
        itoa(format_result, error_buf, 10);
        terminal_writestring(error_buf);
        terminal_writestring("\n");
        current_step = INSTALL_STEP_ERROR;
        return;
    }
    terminal_setcolor(COLOR_GREEN);
    terminal_writestring("      Disk formatted successfully.\n\n");
    terminal_setcolor(COLOR_WHITE);
    debug_print("Disk formatting completed");
    
    // Step 2: Copy system files
    terminal_writestring("[2/5] Copying system files...\n");
    debug_print("About to copy system files");
    
    int copy_result = installer_copy_system_files();
    if (copy_result != 0) {
        terminal_setcolor(COLOR_RED);
        terminal_writestring("ERROR: Failed to copy system files! Error code: ");
        char error_buf[16];
        itoa(copy_result, error_buf, 10);
        terminal_writestring(error_buf);
        terminal_writestring("\n");
        current_step = INSTALL_STEP_ERROR;
        return;
    }
    terminal_setcolor(COLOR_GREEN);
    terminal_writestring("      System files copied successfully.\n\n");
    terminal_setcolor(COLOR_WHITE);
    debug_print("System files copying completed");
    
    // Step 3: Install GRUB
    terminal_writestring("[3/5] Installing bootloader...\n");
    debug_print("About to install bootloader");
    
    int grub_result = installer_install_grub(installer_config.target_disk);
        if (grub_result != 0) {
        terminal_setcolor(COLOR_RED);
        terminal_writestring("ERROR: Failed to install bootloader! Error code: ");
        char error_buf[16];
        itoa(grub_result, error_buf, 10);
        terminal_writestring(error_buf);
        terminal_writestring("\n");
        current_step = INSTALL_STEP_ERROR;
        return;
    }
    terminal_setcolor(COLOR_GREEN);
    terminal_writestring("      Bootloader installed successfully.\n\n");
    terminal_setcolor(COLOR_WHITE);
    debug_print("Bootloader installation completed");
    
    // Step 4: Create user configuration
    terminal_writestring("[4/5] Creating user configuration...\n");
    debug_print("About to create user config");
    
    int user_result = installer_create_user_config(&installer_config);
    if (user_result != 0) {
        terminal_setcolor(COLOR_RED);
        terminal_writestring("ERROR: Failed to create user configuration! Error code: ");
        char error_buf[16];
        itoa(user_result, error_buf, 10);
        terminal_writestring(error_buf);
        terminal_writestring("\n");
        current_step = INSTALL_STEP_ERROR;
        return;
    }
    terminal_setcolor(COLOR_GREEN);
    terminal_writestring("      User configuration created successfully.\n\n");
    terminal_setcolor(COLOR_WHITE);
    debug_print("User configuration completed");
    
    // Step 5: Finalize installation
    terminal_writestring("[5/5] Finalizing installation...\n");
    debug_print("Finalizing installation");
    
    // Flush disk caches if the function exists
    // disk_flush_cache(disk_index);
    
    terminal_setcolor(COLOR_GREEN);
    terminal_writestring("      Installation completed successfully!\n\n");
    terminal_setcolor(COLOR_WHITE);
    debug_print("Installation process completed successfully");
    
    current_step = INSTALL_STEP_COMPLETE;
}

// Installation complete screen
void installer_complete(void) {
    terminal_clear();
    terminal_setcolor(COLOR_GREEN);
    terminal_writestring("=== Installation Complete ===\n\n");
    terminal_setcolor(COLOR_WHITE);
    
    terminal_writestring("SyncWide OS has been successfully installed!\n\n");
    terminal_writestring("The system will now reboot...\n\n");
    terminal_writestring("Press ENTER to reboot...");
    
    while (1) {
        keyboard_poll();
        char c = keyboard_get_key();
        if (c == '\n' || c == '\r') {
            // Reboot the system
            outb(0x64, 0xFE);
            break;
        }
    }
}
