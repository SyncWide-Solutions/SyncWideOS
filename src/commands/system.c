#include "../include/commands.h"
#include <stddef.h>
#include <stdint.h>
#include "../include/string.h"
#include "../include/io.h"

// External function declarations
extern void terminal_writestring(const char* data);
extern size_t strlen(const char* str);

// Multiboot definitions
#define MULTIBOOT_FLAG_MEM     0x001
#define MULTIBOOT_FLAG_DEVICE  0x002
#define MULTIBOOT_FLAG_CMDLINE 0x004
#define MULTIBOOT_FLAG_MODS    0x008
#define MULTIBOOT_FLAG_AOUT    0x010
#define MULTIBOOT_FLAG_ELF     0x020
#define MULTIBOOT_FLAG_MMAP    0x040
#define MULTIBOOT_FLAG_DRIVES  0x080
#define MULTIBOOT_FLAG_CONFIG  0x100
#define MULTIBOOT_FLAG_LOADER  0x200
#define MULTIBOOT_FLAG_APM     0x400
#define MULTIBOOT_FLAG_VBE     0x800

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
} multiboot_info_t;

typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} multiboot_memory_map_t;

// Define multiboot_info variable (will be initialized in kernel.c)
extern multiboot_info_t* multiboot_info;

// Reboot the system using keyboard controller
static void system_reboot(void) {
    terminal_writestring("System is rebooting...\n");
    
    // Wait for keyboard buffer to be empty
    uint8_t temp;
    do {
        temp = inb(0x64);
        if (temp & 1) inb(0x60);
    } while (temp & 2);
    
    // Send reboot command to keyboard controller
    outb(0x64, 0xFE);
    
    // If we get here, the reboot failed
    terminal_writestring("Reboot failed! System halted.\n");
    
    // Halt the CPU
    asm volatile("cli; hlt");
}

// Shutdown the system using ACPI
static void system_shutdown(void) {
    terminal_writestring("System is shutting down...\n");
    
    // ACPI shutdown sequence
    // 1. Check if ACPI is supported (simplified approach)
    // 2. Send the shutdown command to the ACPI port
    
    // ACPI shutdown via Port 0xB004
    outw(0xB004, 0x2000);  // Changed from outb to outw to fix overflow warning
    
    // Try ACPI shutdown via Port 0x604
    outw(0x604, 0x2000);   // Changed from outb to outw to fix overflow warning
    
    // Try APM (Advanced Power Management) shutdown
    outw(0x1004, 0x1234);  // Changed from outb to outw to fix overflow warning
    
    // Try QEMU-specific shutdown
    outb(0x501, 0x0);
    
    // Try Bochs/QEMU-specific shutdown
    outw(0x604, 0x2000);
    
    // Try VirtualBox-specific shutdown
    outw(0x4004, 0x3400);
    
    // If we get here, none of the shutdown methods worked
    terminal_writestring("Shutdown failed! System halted.\n");
    terminal_writestring("It is now safe to turn off your computer.\n");
    
    // Disable interrupts and halt the CPU as a last resort
    asm volatile("cli; hlt");
}

// CPU information detection
static void system_info_cpu(void) {
    terminal_writestring("CPU Information:\n");
    
    // CPU vendor string detection using CPUID
    char vendor[13] = {0};
    uint32_t vendor_registers[4] = {0};
    
    asm volatile("cpuid" 
                : "=a" (vendor_registers[0]),
                  "=b" (vendor_registers[1]),
                  "=c" (vendor_registers[2]),
                  "=d" (vendor_registers[3])
                : "a" (0));
    
    // Construct vendor string from EBX, EDX, ECX
    *((uint32_t*)vendor) = vendor_registers[1];
    *((uint32_t*)(vendor + 4)) = vendor_registers[3];
    *((uint32_t*)(vendor + 8)) = vendor_registers[2];
    vendor[12] = '\0';
    
    terminal_writestring("  Vendor: ");
    terminal_writestring(vendor);
    terminal_writestring("\n");
    
    // Get CPU features
    uint32_t features_ecx = 0;
    uint32_t features_edx = 0;
    
    asm volatile("cpuid"
                : "=c" (features_ecx),
                  "=d" (features_edx)
                : "a" (1)
                : "ebx");
    
    terminal_writestring("  Features: ");
    if (features_edx & (1 << 0)) terminal_writestring("FPU ");
    if (features_edx & (1 << 4)) terminal_writestring("TSC ");
    if (features_edx & (1 << 6)) terminal_writestring("PAE ");
    if (features_edx & (1 << 9)) terminal_writestring("APIC ");
    if (features_edx & (1 << 25)) terminal_writestring("SSE ");
    if (features_ecx & (1 << 0)) terminal_writestring("SSE3 ");
    terminal_writestring("\n");
}

// Memory information from multiboot
static void system_info_memory(void) {
    terminal_writestring("Memory Information:\n");
    
    if (multiboot_info && (multiboot_info->flags & MULTIBOOT_FLAG_MEM)) {
        char buffer[32];
        
        // Lower memory (typically below 1MB)
        terminal_writestring("  Lower memory: ");
        itoa(multiboot_info->mem_lower, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(" KB\n");
        
        // Upper memory (typically above 1MB)
        terminal_writestring("  Upper memory: ");
        itoa(multiboot_info->mem_upper, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(" KB\n");
        
        // Calculate total memory
        uint32_t total_memory = multiboot_info->mem_lower + multiboot_info->mem_upper;
        terminal_writestring("  Total memory: ");
        itoa(total_memory, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(" KB (");
        itoa(total_memory / 1024, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(" MB)\n");
    } else {
        terminal_writestring("  Basic memory information not available\n");
    }
    
    // Memory map information if available
    if (multiboot_info && (multiboot_info->flags & MULTIBOOT_FLAG_MMAP)) {
        terminal_writestring("  Memory map available from multiboot\n");
        
        multiboot_memory_map_t* mmap = (multiboot_memory_map_t*)multiboot_info->mmap_addr;
        uint32_t end = multiboot_info->mmap_addr + multiboot_info->mmap_length;
        
        terminal_writestring("  Memory regions:\n");
        
        while ((uint32_t)mmap < end) {
            char buffer[32];
            
            terminal_writestring("    Region at 0x");
            // Print lower 32 bits of address
            itoa((uint32_t)mmap->addr, buffer, 16);
            terminal_writestring(buffer);
            
            terminal_writestring(", length: 0x");
            // Print lower 32 bits of length
            itoa((uint32_t)mmap->len, buffer, 16);
            terminal_writestring(buffer);
            
            terminal_writestring(", type: ");
            itoa(mmap->type, buffer, 10);
            terminal_writestring(buffer);
            terminal_writestring("\n");
            
            // Move to the next map entry
            mmap = (multiboot_memory_map_t*)((uint32_t)mmap + mmap->size + sizeof(mmap->size));
        }
    }
}

// Boot information from multiboot
static void system_info_boot(void) {
    terminal_writestring("Boot Information:\n");
    
    if (multiboot_info) {
        char buffer[32];
        
        // Boot loader name
        if (multiboot_info->flags & MULTIBOOT_FLAG_LOADER) {
            terminal_writestring("  Bootloader: ");
            terminal_writestring((const char*)multiboot_info->boot_loader_name);
            terminal_writestring("\n");
        }
        
        // Command line
        if (multiboot_info->flags & MULTIBOOT_FLAG_CMDLINE) {
            terminal_writestring("  Command line: ");
            terminal_writestring((const char*)multiboot_info->cmdline);
            terminal_writestring("\n");
        }
        
        // Boot device
        if (multiboot_info->flags & MULTIBOOT_FLAG_DEVICE) {
            terminal_writestring("  Boot device: 0x");
            itoa(multiboot_info->boot_device, buffer, 16);
            terminal_writestring(buffer);
            terminal_writestring("\n");
        }
    } else {
        terminal_writestring("  Boot information not available\n");
    }
}

// Disk information
static void system_info_disk(void) {
    terminal_writestring("Disk Information:\n");
    
    if (multiboot_info && (multiboot_info->flags & MULTIBOOT_FLAG_DRIVES)) {
        char buffer[32];
        
        terminal_writestring("  Drives information available from multiboot\n");
        terminal_writestring("  Drives data at address: 0x");
        itoa(multiboot_info->drives_addr, buffer, 16);
        terminal_writestring(buffer);
        terminal_writestring("\n");
        
        terminal_writestring("  Drives data length: ");
        itoa(multiboot_info->drives_length, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(" bytes\n");
        
        // Note: Detailed drive parsing would require knowledge of the
        // specific drive structure format used by the bootloader
    } else {
        terminal_writestring("  Disk information not available from multiboot\n");
        terminal_writestring("  For detailed disk access, hardware drivers would be needed\n");
    }
}

// System information handler
static void system_info(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check for empty arguments or "usage" command
    if (*args == '\0' || strncmp(args, "usage", 5) == 0) {
        terminal_writestring("Usage: system info [cpu|memory|boot|disk|all]\n");
        terminal_writestring("  cpu    - Display CPU information\n");
        terminal_writestring("  memory - Display memory information\n");
        terminal_writestring("  boot   - Display boot information\n");
        terminal_writestring("  disk   - Display disk information\n");
        terminal_writestring("  all    - Display all system information\n");
        return;
    }
    
    // Check for specific info requests
    if (strncmp(args, "cpu", 3) == 0) {
        system_info_cpu();
        return;
    }
    
    if (strncmp(args, "memory", 6) == 0) {
        system_info_memory();
        return;
    }
    
    if (strncmp(args, "boot", 4) == 0) {
        system_info_boot();
        return;
    }
    
    if (strncmp(args, "disk", 4) == 0) {
        system_info_disk();
        return;
    }
    
    if (strncmp(args, "all", 3) == 0) {
        system_info_cpu();
        terminal_writestring("\n");
        system_info_memory();
        terminal_writestring("\n");
        system_info_boot();
        terminal_writestring("\n");
        system_info_disk();
        return;
    }
    
    // Unknown argument
    terminal_writestring("Unknown info command: ");
    terminal_writestring(args);
    terminal_writestring("\n");
    terminal_writestring("Try 'system info usage' for available options\n");
}

// System command handler
void cmd_system(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check for empty arguments
    if (*args == '\0') {
        terminal_writestring("Usage: system [restart|reboot|shutdown|powerdown|info]\n");
        return;
    }
    
    // Check for restart/reboot command
    if (strncmp(args, "restart", 7) == 0 || strncmp(args, "reboot", 6) == 0) {
        system_reboot();
        return;
    }
    
    // Check for shutdown/powerdown command
    if (strncmp(args, "shutdown", 8) == 0 || strncmp(args, "powerdown", 9) == 0) {
        system_shutdown();
        return;
    }
    
    // Check for info command
    if (strncmp(args, "info", 4) == 0) {
        // Pass the rest of the arguments to the info handler
        system_info(args + 4);
        return;
    }
    
    // Unknown argument
    terminal_writestring("Unknown system command: ");
    terminal_writestring(args);
    terminal_writestring("\n");
    terminal_writestring("Available commands: restart, reboot, shutdown, powerdown, info\n");
}
