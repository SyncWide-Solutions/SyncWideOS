#include "../include/commands.h"
#include <stddef.h>
#include <stdint.h>
#include "../include/string.h"
#include "../include/io.h"
#include "../include/disk.h"
#include "../include/gpu.h"

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

// CPU information structure
typedef struct {
    char vendor[13];
    char brand[49];
    uint32_t family;
    uint32_t model;
    uint32_t stepping;
    uint32_t type;
    uint32_t extended_family;
    uint32_t extended_model;
    uint32_t max_cpuid;
    uint32_t max_extended_cpuid;
    uint32_t features_edx;
    uint32_t features_ecx;
    uint32_t extended_features_edx;
    uint32_t extended_features_ecx;
    uint32_t cache_info[4];
} cpu_info_t;

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

// Check if CPUID is supported
static int cpuid_supported(void) {
    uint32_t eflags_before, eflags_after;
    
    // Try to flip the ID bit (bit 21) in EFLAGS
    asm volatile(
        "pushfl\n\t"
        "popl %0\n\t"
        "movl %0, %1\n\t"
        "xorl $0x200000, %0\n\t"
        "pushl %0\n\t"
        "popfl\n\t"
        "pushfl\n\t"
        "popl %0\n\t"
        "pushl %1\n\t"
        "popfl"
        : "=&r" (eflags_after), "=&r" (eflags_before)
        :
        : "cc"
    );
    
    return ((eflags_before ^ eflags_after) & 0x200000) != 0;
}

static void cpuid(uint32_t leaf, uint32_t subleaf, uint32_t* eax, uint32_t* ebx, uint32_t* ecx, uint32_t* edx) {
    asm volatile("cpuid"
                : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
                : "a" (leaf), "c" (subleaf));
}

static void get_cpu_vendor(cpu_info_t* cpu) {
    uint32_t eax, ebx, ecx, edx;
    cpuid(0, 0, &eax, &ebx, &ecx, &edx);
    
    cpu->max_cpuid = eax;
    
    // Vendor string is in EBX, EDX, ECX order
    *((uint32_t*)cpu->vendor) = ebx;
    *((uint32_t*)(cpu->vendor + 4)) = edx;
    *((uint32_t*)(cpu->vendor + 8)) = ecx;
    cpu->vendor[12] = '\0';
}

static void get_cpu_brand(cpu_info_t* cpu) {
    uint32_t eax, ebx, ecx, edx;
    
    // Check if extended CPUID is supported
    cpuid(0x80000000, 0, &eax, &ebx, &ecx, &edx);
    cpu->max_extended_cpuid = eax;
    
    if (eax < 0x80000004) {
        // Brand string not supported
        cpu->brand[0] = '\0';
        return;
    }
    
    // Get brand string from leaves 0x80000002, 0x80000003, 0x80000004
    cpuid(0x80000002, 0, &eax, &ebx, &ecx, &edx);
    *((uint32_t*)(cpu->brand + 0)) = eax;
    *((uint32_t*)(cpu->brand + 4)) = ebx;
    *((uint32_t*)(cpu->brand + 8)) = ecx;
    *((uint32_t*)(cpu->brand + 12)) = edx;
    
    cpuid(0x80000003, 0, &eax, &ebx, &ecx, &edx);
    *((uint32_t*)(cpu->brand + 16)) = eax;
    *((uint32_t*)(cpu->brand + 20)) = ebx;
    *((uint32_t*)(cpu->brand + 24)) = ecx;
    *((uint32_t*)(cpu->brand + 28)) = edx;
    
    cpuid(0x80000004, 0, &eax, &ebx, &ecx, &edx);
    *((uint32_t*)(cpu->brand + 32)) = eax;
    *((uint32_t*)(cpu->brand + 36)) = ebx;
    *((uint32_t*)(cpu->brand + 40)) = ecx;
    *((uint32_t*)(cpu->brand + 44)) = edx;
    
    cpu->brand[48] = '\0';
}

// Get CPU family, model, stepping information
static void get_cpu_signature(cpu_info_t* cpu) {
    uint32_t eax, ebx, ecx, edx;
    
    if (cpu->max_cpuid < 1) return;
    
    cpuid(1, 0, &eax, &ebx, &ecx, &edx);
    
    cpu->features_edx = edx;
    cpu->features_ecx = ecx;
    
    // Extract family, model, stepping from EAX
    cpu->stepping = eax & 0xF;
    cpu->model = (eax >> 4) & 0xF;
    cpu->family = (eax >> 8) & 0xF;
    cpu->type = (eax >> 12) & 0x3;
    cpu->extended_model = (eax >> 16) & 0xF;
    cpu->extended_family = (eax >> 20) & 0xFF;
    
    // Calculate display family and model
    if (cpu->family == 0xF) {
        cpu->family += cpu->extended_family;
    }
    
    if (cpu->family == 0x6 || cpu->family == 0xF) {
        cpu->model += (cpu->extended_model << 4);
    }
}

// Get extended CPU features
static void get_extended_features(cpu_info_t* cpu) {
    uint32_t eax, ebx, ecx, edx;
    
    if (cpu->max_extended_cpuid >= 0x80000001) {
        cpuid(0x80000001, 0, &eax, &ebx, &ecx, &edx);
        cpu->extended_features_edx = edx;
        cpu->extended_features_ecx = ecx;
    } else {
        cpu->extended_features_edx = 0;
        cpu->extended_features_ecx = 0;
    }
}

// Get cache information
static void get_cache_info(cpu_info_t* cpu) {
    if (cpu->max_cpuid >= 2) {
        cpuid(2, 0, &cpu->cache_info[0], &cpu->cache_info[1], 
              &cpu->cache_info[2], &cpu->cache_info[3]);
    } else {
        cpu->cache_info[0] = cpu->cache_info[1] = 
        cpu->cache_info[2] = cpu->cache_info[3] = 0;
    }
}

// Print CPU features
static void print_cpu_features(const cpu_info_t* cpu) {
    terminal_writestring("  Standard Features: ");
    
    if (cpu->features_edx & (1 << 0)) terminal_writestring("FPU ");
    if (cpu->features_edx & (1 << 1)) terminal_writestring("VME ");
    if (cpu->features_edx & (1 << 2)) terminal_writestring("DE ");
    if (cpu->features_edx & (1 << 3)) terminal_writestring("PSE ");
    if (cpu->features_edx & (1 << 4)) terminal_writestring("TSC ");
    if (cpu->features_edx & (1 << 5)) terminal_writestring("MSR ");
    if (cpu->features_edx & (1 << 6)) terminal_writestring("PAE ");
    if (cpu->features_edx & (1 << 7)) terminal_writestring("MCE ");
    if (cpu->features_edx & (1 << 8)) terminal_writestring("CX8 ");
    if (cpu->features_edx & (1 << 9)) terminal_writestring("APIC ");
    if (cpu->features_edx & (1 << 11)) terminal_writestring("SEP ");
    if (cpu->features_edx & (1 << 12)) terminal_writestring("MTRR ");
    if (cpu->features_edx & (1 << 13)) terminal_writestring("PGE ");
    if (cpu->features_edx & (1 << 14)) terminal_writestring("MCA ");
    if (cpu->features_edx & (1 << 15)) terminal_writestring("CMOV ");
    if (cpu->features_edx & (1 << 16)) terminal_writestring("PAT ");
    if (cpu->features_edx & (1 << 17)) terminal_writestring("PSE-36 ");
    if (cpu->features_edx & (1 << 19)) terminal_writestring("CLFSH ");
    if (cpu->features_edx & (1 << 23)) terminal_writestring("MMX ");
    if (cpu->features_edx & (1 << 24)) terminal_writestring("FXSR ");
    if (cpu->features_edx & (1 << 25)) terminal_writestring("SSE ");
    if (cpu->features_edx & (1 << 26)) terminal_writestring("SSE2 ");
    if (cpu->features_edx & (1 << 28)) terminal_writestring("HTT ");
    
    terminal_writestring("\n  Extended Features: ");
    
    if (cpu->features_ecx & (1 << 0)) terminal_writestring("SSE3 ");
    if (cpu->features_ecx & (1 << 1)) terminal_writestring("PCLMULQDQ ");
    if (cpu->features_ecx & (1 << 3)) terminal_writestring("MONITOR ");
    if (cpu->features_ecx & (1 << 9)) terminal_writestring("SSSE3 ");
    if (cpu->features_ecx & (1 << 12)) terminal_writestring("FMA ");
    if (cpu->features_ecx & (1 << 13)) terminal_writestring("CMPXCHG16B ");
    if (cpu->features_ecx & (1 << 19)) terminal_writestring("SSE4.1 ");
    if (cpu->features_ecx & (1 << 20)) terminal_writestring("SSE4.2 ");
    if (cpu->features_ecx & (1 << 22)) terminal_writestring("MOVBE ");
    if (cpu->features_ecx & (1 << 23)) terminal_writestring("POPCNT ");
    if (cpu->features_ecx & (1 << 25)) terminal_writestring("AES ");
    if (cpu->features_ecx & (1 << 26)) terminal_writestring("XSAVE ");
    if (cpu->features_ecx & (1 << 28)) terminal_writestring("AVX ");
    
    terminal_writestring("\n  AMD Extended Features: ");
    
    if (cpu->extended_features_edx & (1 << 11)) terminal_writestring("SYSCALL ");
    if (cpu->extended_features_edx & (1 << 20)) terminal_writestring("NX ");
    if (cpu->extended_features_edx & (1 << 22)) terminal_writestring("MMXEXT ");
    if (cpu->extended_features_edx & (1 << 25)) terminal_writestring("FXSR_OPT ");
    if (cpu->extended_features_edx & (1 << 26)) terminal_writestring("PDPE1GB ");
    if (cpu->extended_features_edx & (1 << 27)) terminal_writestring("RDTSCP ");
    if (cpu->extended_features_edx & (1 << 29)) terminal_writestring("LM ");
    if (cpu->extended_features_edx & (1 << 31)) terminal_writestring("3DNOWEXT ");
    if (cpu->extended_features_edx & (1 << 30)) terminal_writestring("3DNOW ");
    
    terminal_writestring("\n");
}

// CPU information detection
// Enhanced CPU information detection
static void system_info_cpu(void) {
    terminal_writestring("CPU Information:\n");
    
    // Check if CPUID is supported
    if (!cpuid_supported()) {
        terminal_writestring("  CPUID instruction not supported on this processor\n");
        terminal_writestring("  This appears to be a very old CPU (pre-486)\n");
        return;
    }
    
    cpu_info_t cpu = {0};
    char buffer[32];
    
    // Get all CPU information
    get_cpu_vendor(&cpu);
    get_cpu_brand(&cpu);
    get_cpu_signature(&cpu);
    get_extended_features(&cpu);
    get_cache_info(&cpu);
    
    // Display vendor
    terminal_writestring("  Vendor: ");
    terminal_writestring(cpu.vendor);
    terminal_writestring("\n");
    
    // Display brand string if available
    if (cpu.brand[0] != '\0') {
        terminal_writestring("  Brand: ");
        // Trim leading spaces from brand string
        const char* brand_start = cpu.brand;
        while (*brand_start == ' ') brand_start++;
        terminal_writestring(brand_start);
        terminal_writestring("\n");
    }
    
    // Display family, model, stepping
    terminal_writestring("  Family: ");
    itoa(cpu.family, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring(", Model: ");
    itoa(cpu.model, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring(", Stepping: ");
    itoa(cpu.stepping, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring("\n");
    
    // Display processor type
    terminal_writestring("  Type: ");
    switch (cpu.type) {
        case 0:
            terminal_writestring("Original OEM Processor");
            break;
        case 1:
            terminal_writestring("Intel OverDrive Processor");
            break;
        case 2:
            terminal_writestring("Dual Processor");
            break;
        case 3:
            terminal_writestring("Intel Reserved");
            break;
        default:
            terminal_writestring("Unknown");
            break;
    }
    terminal_writestring("\n");
    
    // Display CPUID support levels
    terminal_writestring("  Max CPUID Level: 0x");
    itoa(cpu.max_cpuid, buffer, 16);
    terminal_writestring(buffer);
    if (cpu.max_extended_cpuid > 0) {
        terminal_writestring(", Max Extended Level: 0x");
        itoa(cpu.max_extended_cpuid, buffer, 16);
        terminal_writestring(buffer);
    }
    terminal_writestring("\n");
    
    // Display architecture information
    terminal_writestring("  Architecture: ");
    if (cpu.extended_features_edx & (1 << 29)) {
        terminal_writestring("x86-64 (64-bit capable)");
    } else {
        terminal_writestring("x86 (32-bit)");
    }
    terminal_writestring("\n");
    
    // Display cache information (simplified)
    if (cpu.cache_info[0] != 0) {
        terminal_writestring("  Cache Info Available: Yes (use detailed cache command for more info)\n");
    }
    
    // Display feature flags
    print_cpu_features(&cpu);
    
    // Display specific CPU identification
    terminal_writestring("  Identified as: ");
    if (strncmp(cpu.vendor, "GenuineIntel", 12) == 0) {
        terminal_writestring("Intel ");
        // Intel-specific identification
        if (cpu.family == 4) {
            terminal_writestring("486");
        } else if (cpu.family == 5) {
            terminal_writestring("Pentium");
        } else if (cpu.family == 6) {
            switch (cpu.model) {
                case 1: terminal_writestring("Pentium Pro"); break;
                case 3: terminal_writestring("Pentium II"); break;
                case 5: terminal_writestring("Pentium II/Celeron"); break;
                case 6: terminal_writestring("Celeron"); break;
                case 7: terminal_writestring("Pentium III"); break;
                case 8: terminal_writestring("Pentium III"); break;
                case 9: terminal_writestring("Pentium M"); break;
                case 10: terminal_writestring("Pentium III Xeon"); break;
                case 11: terminal_writestring("Pentium III"); break;
                case 13: terminal_writestring("Pentium M"); break;
                case 14: terminal_writestring("Core"); break;
                case 15: terminal_writestring("Core 2"); break;
                case 23: terminal_writestring("Core 2 Extreme"); break;
                case 26: terminal_writestring("Core i7"); break;
                case 28: terminal_writestring("Atom"); break;
                case 30: terminal_writestring("Core i7"); break;
                case 37: terminal_writestring("Core i5/i7"); break;
                case 42: terminal_writestring("Core i3/i5/i7 (Sandy Bridge)"); break;
                case 58: terminal_writestring("Core i3/i5/i7 (Ivy Bridge)"); break;
                case 60: terminal_writestring("Core i3/i5/i7 (Haswell)"); break;
                case 61: terminal_writestring("Core i3/i5/i7 (Broadwell)"); break;
                case 78: terminal_writestring("Core i3/i5/i7 (Skylake)"); break;
                case 94: terminal_writestring("Core i3/i5/i7 (Skylake)"); break;
                case 142: terminal_writestring("Core i3/i5/i7 (Kaby Lake)"); break;
                case 158: terminal_writestring("Core i3/i5/i7 (Kaby Lake)"); break;
                default:
                    terminal_writestring("Unknown Intel Model ");
                    itoa(cpu.model, buffer, 10);
                    terminal_writestring(buffer);
                    break;
            }
        } else if (cpu.family == 15) {
            terminal_writestring("Pentium 4");
        } else {
            terminal_writestring("Unknown Intel Family ");
            itoa(cpu.family, buffer, 10);
            terminal_writestring(buffer);
        }
    } else if (strncmp(cpu.vendor, "AuthenticAMD", 12) == 0) {
        terminal_writestring("AMD ");
        // AMD-specific identification
        if (cpu.family == 4) {
            terminal_writestring("486");
        } else if (cpu.family == 5) {
            terminal_writestring("K5/K6");
        } else if (cpu.family == 6) {
            terminal_writestring("K7 (Athlon/Duron)");
        } else if (cpu.family == 15) {
            terminal_writestring("K8 (Athlon 64/Opteron)");
        } else if (cpu.family == 16) {
            terminal_writestring("K10 (Phenom)");
        } else if (cpu.family == 17) {
            terminal_writestring("K10");
        } else if (cpu.family == 18) {
            terminal_writestring("K10 (Llano)");
        } else if (cpu.family == 20) {
            terminal_writestring("Bobcat");
        } else if (cpu.family == 21) {
            terminal_writestring("Bulldozer/Piledriver");
        } else if (cpu.family == 22) {
            terminal_writestring("Jaguar");
        } else if (cpu.family == 23) {
            terminal_writestring("Zen/Zen+");
        } else if (cpu.family == 25) {
            terminal_writestring("Zen 3");
        } else {
            terminal_writestring("Unknown AMD Family ");
            itoa(cpu.family, buffer, 10);
            terminal_writestring(buffer);
        }
    } else if (strncmp(cpu.vendor, "CyrixInstead", 12) == 0) {
        terminal_writestring("Cyrix Processor");
    } else if (strncmp(cpu.vendor, "CentaurHauls", 12) == 0) {
        terminal_writestring("VIA/Centaur Processor");
    } else if (strncmp(cpu.vendor, "GenuineTMx86", 12) == 0) {
        terminal_writestring("Transmeta Processor");
    } else if (strncmp(cpu.vendor, "Geode by NSC", 12) == 0) {
        terminal_writestring("National Semiconductor Geode");
    } else if (strncmp(cpu.vendor, "NexGenDriven", 12) == 0) {
        terminal_writestring("NexGen Processor");
    } else if (strncmp(cpu.vendor, "RiseRiseRise", 12) == 0) {
        terminal_writestring("Rise Technology Processor");
    } else if (strncmp(cpu.vendor, "SiS SiS SiS ", 12) == 0) {
        terminal_writestring("SiS Processor");
    } else if (strncmp(cpu.vendor, "UMC UMC UMC ", 12) == 0) {
        terminal_writestring("UMC Processor");
    } else if (strncmp(cpu.vendor, "VIA VIA VIA ", 12) == 0) {
        terminal_writestring("VIA Processor");
    } else if (strncmp(cpu.vendor, "Vortex86 SoC", 12) == 0) {
        terminal_writestring("Vortex86 SoC");
    } else if (strncmp(cpu.vendor, "KVMKVMKVM", 9) == 0) {
        terminal_writestring("KVM Virtual CPU");
    } else if (strncmp(cpu.vendor, "Microsoft Hv", 12) == 0) {
        terminal_writestring("Microsoft Hyper-V");
    } else if (strncmp(cpu.vendor, "VMwareVMware", 12) == 0) {
        terminal_writestring("VMware Virtual CPU");
    } else if (strncmp(cpu.vendor, "XenVMMXenVMM", 12) == 0) {
        terminal_writestring("Xen Virtual CPU");
    } else if (strncmp(cpu.vendor, "TCGTCGTCGTCG", 12) == 0) {
        terminal_writestring("QEMU Virtual CPU");
    } else {
        terminal_writestring("Unknown/Other (");
        terminal_writestring(cpu.vendor);
        terminal_writestring(")");
    }
    terminal_writestring("\n");
    
    // Performance and power management features
    terminal_writestring("  Power Management: ");
    if (cpu.features_ecx & (1 << 3)) terminal_writestring("MONITOR/MWAIT ");
    if (cpu.features_ecx & (1 << 7)) terminal_writestring("SpeedStep ");
    if (cpu.extended_features_ecx & (1 << 3)) terminal_writestring("SVM ");
    if (cpu.features_ecx & (1 << 5)) terminal_writestring("VMX ");
    terminal_writestring("\n");
    
    // Security features
    terminal_writestring("  Security Features: ");
    if (cpu.extended_features_edx & (1 << 20)) terminal_writestring("NX-bit ");
    if (cpu.features_ecx & (1 << 25)) terminal_writestring("AES-NI ");
    if (cpu.features_ecx & (1 << 1)) terminal_writestring("PCLMULQDQ ");
    if (cpu.features_ecx & (1 << 30)) terminal_writestring("RDRAND ");
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

// Disk Information
static void system_info_disk(void) {
    terminal_writestring("Disk Information:\n");
    
    // Initialize disk subsystem if not already done
    static int disk_initialized = 0;
    if (!disk_initialized) {
        terminal_writestring("  Initializing disk subsystem...\n");
        disk_init();
        disk_initialized = 1;
    }
    
    // Get number of detected drives
    int drive_count = disk_get_drive_count();
    char buffer[32];
    
    terminal_writestring("  Detected drives: ");
    itoa(drive_count, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring("\n");
    
    if (drive_count == 0) {
        terminal_writestring("  No drives detected.\n");
        terminal_writestring("  This could be due to:\n");
        terminal_writestring("    - No physical drives connected\n");
        terminal_writestring("    - Drives not compatible with ATA/IDE interface\n");
        terminal_writestring("    - Hardware initialization issues\n");
        return;
    }
    
    terminal_writestring("\n");
    
    // Print detailed information for each drive
    for (int i = 0; i < 4; i++) {
        ide_device_t* drive = disk_get_drive_info(i);
        if (drive != NULL) {
            disk_print_drive_info(i);
            terminal_writestring("\n");
        }
    }
    
    // Legacy multiboot disk information (if available)
    if (multiboot_info && (multiboot_info->flags & MULTIBOOT_FLAG_DRIVES)) {
        terminal_writestring("  Additional multiboot drive information:\n");
        terminal_writestring("    Drives data at address: 0x");
        itoa(multiboot_info->drives_addr, buffer, 16);
        terminal_writestring(buffer);
        terminal_writestring("\n");
        
        terminal_writestring("    Drives data length: ");
        itoa(multiboot_info->drives_length, buffer, 10);
        terminal_writestring(buffer);
        terminal_writestring(" bytes\n");
    }
    
    // Show IDE controller information
    terminal_writestring("  IDE Controller Information:\n");
    terminal_writestring("    Primary channel: I/O 0x1F0-0x1F7, Control 0x3F6\n");
    terminal_writestring("    Secondary channel: I/O 0x170-0x177, Control 0x376\n");
    terminal_writestring("    Supports: ATA/ATAPI devices, PIO mode\n");
    terminal_writestring("    Note: DMA mode not implemented\n");
}

// GPU information
static void system_info_gpu(void) {
    terminal_writestring("GPU Information:\n");
    
    // Initialize GPU subsystem if not already done
    gpu_init();
    
    int gpu_count = gpu_get_device_count();
    
    if (gpu_count == 0) {
        terminal_writestring("  No GPU devices detected\n");
        terminal_writestring("  Note: This may indicate:\n");
        terminal_writestring("    - No graphics hardware present\n");
        terminal_writestring("    - Graphics hardware not PCI-based\n");
        terminal_writestring("    - Driver detection issues\n");
        return;
    }
    
    char buffer[32];
    terminal_writestring("  Total GPU devices found: ");
    itoa(gpu_count, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring("\n\n");
    
    // Display information for each GPU
    for (int i = 0; i < gpu_count; i++) {
        gpu_device_t* gpu = gpu_get_device_info(i);
        if (gpu && gpu->present) {
            terminal_writestring("GPU ");
            itoa(i, buffer, 10);
            terminal_writestring(buffer);
            if (gpu->primary) {
                terminal_writestring(" (Primary Display Adapter)");
            }
            terminal_writestring(":\n");
            
            // Basic information
            terminal_writestring("  Vendor: ");
            terminal_writestring(gpu->vendor_name);
            terminal_writestring("\n");
            
            terminal_writestring("  Device: ");
            terminal_writestring(gpu->device_name);
            terminal_writestring("\n");
            
            terminal_writestring("  Type: ");
            switch (gpu->gpu_type) {
                case GPU_TYPE_VGA:
                    terminal_writestring("VGA Compatible Controller");
                    break;
                case GPU_TYPE_SVGA:
                    terminal_writestring("Super VGA Controller");
                    break;
                case GPU_TYPE_ACCELERATED:
                    terminal_writestring("Hardware Accelerated Graphics");
                    break;
                case GPU_TYPE_INTEGRATED:
                    terminal_writestring("Integrated Graphics Processor");
                    break;
                case GPU_TYPE_DISCRETE:
                    terminal_writestring("Discrete Graphics Card");
                    break;
                case GPU_TYPE_VIRTUAL:
                    terminal_writestring("Virtual Graphics Adapter");
                    break;
                default:
                    terminal_writestring("Unknown Graphics Device");
                    break;
            }
            terminal_writestring("\n");
            
            // Memory information
            terminal_writestring("  Video Memory: ");
            if (gpu->memory_size >= 1024) {
                itoa(gpu->memory_size / 1024, buffer, 10);
                terminal_writestring(buffer);
                terminal_writestring(" GB");
            } else {
                itoa(gpu->memory_size, buffer, 10);
                terminal_writestring(buffer);
                terminal_writestring(" MB");
            }
            
            terminal_writestring(" (");
            switch (gpu->memory_type) {
                case GPU_MEM_SHARED:
                    terminal_writestring("Shared System Memory");
                    break;
                case GPU_MEM_DEDICATED:
                    terminal_writestring("Dedicated Video Memory");
                    break;
                case GPU_MEM_UNIFIED:
                    terminal_writestring("Unified Memory Architecture");
                    break;
                default:
                    terminal_writestring("Unknown Memory Type");
                    break;
            }
            terminal_writestring(")\n");
            
            // Current display mode
            terminal_writestring("  Current Display Mode: ");
            itoa(gpu->current_width, buffer, 10);
            terminal_writestring(buffer);
            terminal_writestring("x");
            itoa(gpu->current_height, buffer, 10);
            terminal_writestring(buffer);
            
            if (gpu->current_bpp <= 8) {
                terminal_writestring(" (");
                itoa(gpu->current_bpp, buffer, 10);
                terminal_writestring(buffer);
                terminal_writestring("-bit color)");
            } else {
                terminal_writestring("x");
                itoa(gpu->current_bpp, buffer, 10);
                terminal_writestring(buffer);
                terminal_writestring("-bit");
            }
            
            terminal_writestring(" @ ");
            itoa(gpu->current_refresh, buffer, 10);
            terminal_writestring(buffer);
            terminal_writestring("Hz\n");
            
            // Hardware capabilities
            terminal_writestring("  Hardware Features: ");
            if (gpu->capabilities & GPU_CAP_2D) terminal_writestring("2D ");
            if (gpu->capabilities & GPU_CAP_3D) terminal_writestring("3D ");
            if (gpu->capabilities & GPU_CAP_HARDWARE_ACCEL) terminal_writestring("Hardware-Acceleration ");
            if (gpu->capabilities & GPU_CAP_SHADER) terminal_writestring("Programmable-Shaders ");
            terminal_writestring("\n");
            
            // API support
            terminal_writestring("  Graphics APIs: ");
            if (gpu->capabilities & GPU_CAP_DIRECTX) terminal_writestring("DirectX ");
            if (gpu->capabilities & GPU_CAP_OPENGL) terminal_writestring("OpenGL ");
            if (gpu->capabilities & GPU_CAP_VULKAN) terminal_writestring("Vulkan ");
            if (gpu->capabilities & GPU_CAP_COMPUTE) terminal_writestring("Compute-Shaders ");
            terminal_writestring("\n");
            
            // Video capabilities
            terminal_writestring("  Video Features: ");
            if (gpu->capabilities & GPU_CAP_VIDEO_DECODE) terminal_writestring("Hardware-Decode ");
            if (gpu->capabilities & GPU_CAP_VIDEO_ENCODE) terminal_writestring("Hardware-Encode ");
            if (!(gpu->capabilities & (GPU_CAP_VIDEO_DECODE | GPU_CAP_VIDEO_ENCODE))) {
                terminal_writestring("Software-Only ");
            }
            terminal_writestring("\n");
            
            // Display connectivity
            terminal_writestring("  Display Outputs: ");
            if (gpu->capabilities & GPU_CAP_VGA_COMPAT) terminal_writestring("VGA ");
            if (gpu->capabilities & GPU_CAP_HDMI) terminal_writestring("HDMI ");
            if (gpu->capabilities & GPU_CAP_DISPLAYPORT) terminal_writestring("DisplayPort ");
            if (gpu->capabilities & GPU_CAP_MULTI_MONITOR) terminal_writestring("Multi-Monitor ");
            terminal_writestring("\n");
            
            // Device IDs
            terminal_writestring("  Device IDs: Vendor=0x");
            itoa(gpu->vendor_id, buffer, 16);
            terminal_writestring(buffer);
            terminal_writestring(", Device=0x");
            itoa(gpu->device_id, buffer, 16);
            terminal_writestring(buffer);
            terminal_writestring(", Revision=0x");
            itoa(gpu->revision, buffer, 16);
            terminal_writestring(buffer);
            terminal_writestring("\n");
            
            // Driver information
            terminal_writestring("  Driver Version: ");
            terminal_writestring(gpu->driver_version);
            terminal_writestring(" (SyncWideOS Generic Driver)\n");
            
            if (i < gpu_count - 1) {
                terminal_writestring("\n");
            }
        }
    }
    
    // Additional system graphics information
    terminal_writestring("\nGraphics System Status:\n");
    terminal_writestring("  VGA Text Mode: Active (80x25)\n");
    terminal_writestring("  Graphics Mode: Not Active\n");
    terminal_writestring("  Hardware Acceleration: ");
    
    // Check if any GPU supports hardware acceleration
    int has_hw_accel = 0;
    for (int i = 0; i < gpu_count; i++) {
        gpu_device_t* gpu = gpu_get_device_info(i);
        if (gpu && (gpu->capabilities & GPU_CAP_HARDWARE_ACCEL)) {
            has_hw_accel = 1;
            break;
        }
    }
    
    if (has_hw_accel) {
        terminal_writestring("Available (Not Initialized)\n");
    } else {
        terminal_writestring("Not Available\n");
    }
    
    terminal_writestring("  Frame Buffer: VGA Compatible (0xB8000)\n");
    terminal_writestring("  Color Depth: 4-bit (16 colors)\n");
}


// System information handler
static void system_info(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check for empty arguments or "usage" command
    if (*args == '\0' || strncmp(args, "usage", 5) == 0) {
        terminal_writestring("Usage: system info [cpu|memory|boot|disk|gpu|all]\n");
        terminal_writestring("  cpu    - Display CPU information\n");
        terminal_writestring("  memory - Display memory information\n");
        terminal_writestring("  boot   - Display boot information\n");
        terminal_writestring("  disk   - Display disk information\n");
        terminal_writestring("  gpu    - Display GPU information\n");
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
    
    if (strncmp(args, "gpu", 3) == 0) {
        system_info_gpu();
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
        terminal_writestring("\n");
        system_info_gpu();
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
