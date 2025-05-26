#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>  // For NULL
#include "../include/pci.h"
#include "../include/io.h"

#define PCI_CONFIG_ADDRESS 0xCF8
#define PCI_CONFIG_DATA    0xCFC

// Global variables to store found devices
static pci_device_t pci_devices[32]; // Support up to 32 PCI devices
static int pci_device_count = 0;

static uint32_t pci_config_address(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    return (uint32_t)((bus << 16) | (slot << 11) | (func << 8) | (offset & 0xFC) | 0x80000000);
}

uint8_t pci_config_read_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = pci_config_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inb(PCI_CONFIG_DATA + (offset & 3));
}

uint16_t pci_config_read_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = pci_config_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inw(PCI_CONFIG_DATA + (offset & 2));
}

uint32_t pci_config_read_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset) {
    uint32_t address = pci_config_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

void pci_config_write_byte(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint8_t value) {
    uint32_t address = pci_config_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outb(PCI_CONFIG_DATA + (offset & 3), value);
}

void pci_config_write_word(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint16_t value) {
    uint32_t address = pci_config_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outw(PCI_CONFIG_DATA + (offset & 2), value);
}

void pci_config_write_dword(uint8_t bus, uint8_t slot, uint8_t func, uint8_t offset, uint32_t value) {
    uint32_t address = pci_config_address(bus, slot, func, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

// Helper function to check if a device exists
static bool pci_device_exists(uint8_t bus, uint8_t slot, uint8_t func) {
    uint16_t vendor_id = pci_config_read_word(bus, slot, func, PCI_VENDOR_ID);
    return (vendor_id != 0xFFFF);
}

// Helper function to get device class information
static void pci_get_device_info(uint8_t bus, uint8_t slot, uint8_t func, pci_device_t* device) {
    device->bus = bus;
    device->slot = slot;
    device->func = func;
    
    // Read vendor and device ID
    uint32_t vendor_device = pci_config_read_dword(bus, slot, func, PCI_VENDOR_ID);
    device->vendor_id = vendor_device & 0xFFFF;
    device->device_id = (vendor_device >> 16) & 0xFFFF;
    
    // Read class code information
    uint32_t class_info = pci_config_read_dword(bus, slot, func, PCI_REVISION_ID);
    device->prog_if = (class_info >> 8) & 0xFF;
    device->subclass = (class_info >> 16) & 0xFF;
    device->class_code = (class_info >> 24) & 0xFF;
    
    // Read BAR registers
    for (int i = 0; i < 6; i++) {
        device->bar[i] = pci_config_read_dword(bus, slot, func, PCI_BAR0 + (i * 4));
    }
    
    // Read interrupt information
    device->interrupt_line = pci_config_read_byte(bus, slot, func, PCI_INTERRUPT_LINE);
    device->interrupt_pin = pci_config_read_byte(bus, slot, func, PCI_INTERRUPT_PIN);
}

// Helper function to get device type string
static const char* pci_get_class_name(uint8_t class_code) {
    switch (class_code) {
        case 0x00: return "Unclassified";
        case 0x01: return "Mass Storage";
        case 0x02: return "Network";
        case 0x03: return "Display";
        case 0x04: return "Multimedia";
        case 0x05: return "Memory";
        case 0x06: return "Bridge";
        case 0x07: return "Communication";
        case 0x08: return "System Peripheral";
        case 0x09: return "Input Device";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus";
        case 0x0D: return "Wireless";
        case 0x0E: return "Intelligent I/O";
        case 0x0F: return "Satellite Communication";
        case 0x10: return "Encryption/Decryption";
        case 0x11: return "Data Acquisition";
        default: return "Unknown";
    }
}

// Helper function to get vendor name
static const char* pci_get_vendor_name(uint16_t vendor_id) {
    switch (vendor_id) {
        case 0x8086: return "Intel";
        case 0x1022: return "AMD";
        case 0x10DE: return "NVIDIA";
        case 0x1002: return "ATI/AMD";
        case 0x10EC: return "Realtek";
        case 0x1106: return "VIA";
        case 0x1274: return "Ensoniq";
        case 0x15AD: return "VMware";
        case 0x80EE: return "VirtualBox";
        case 0x1234: return "QEMU";
        default: return "Unknown";
    }
}

void pci_scan_bus(void) {
    extern void terminal_writestring(const char* data);
    
    pci_device_count = 0;
    
    terminal_writestring("Scanning PCI bus...\n");
    
    // Scan all possible PCI locations
    for (uint16_t bus = 0; bus < 256; bus++) {
        for (uint8_t slot = 0; slot < 32; slot++) {
            for (uint8_t func = 0; func < 8; func++) {
                if (pci_device_exists(bus, slot, func)) {
                    if (pci_device_count < 32) {
                        pci_device_t* device = &pci_devices[pci_device_count];
                        pci_get_device_info(bus, slot, func, device);
                        
                        // Print device information
                        terminal_writestring("Found PCI device: ");
                        terminal_writestring(pci_get_vendor_name(device->vendor_id));
                        terminal_writestring(" ");
                        terminal_writestring(pci_get_class_name(device->class_code));
                        
                        // Print vendor:device ID
                        terminal_writestring(" (");
                        char hex_str[16];
                        
                        // Convert vendor ID to hex string
                        uint16_t vid = device->vendor_id;
                        hex_str[0] = '0'; hex_str[1] = 'x';
                        for (int i = 3; i >= 0; i--) {
                            uint8_t nibble = (vid >> (i * 4)) & 0xF;
                            hex_str[6-i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
                        }
                        hex_str[6] = ':';
                        
                        // Convert device ID to hex string
                        uint16_t did = device->device_id;
                        for (int i = 3; i >= 0; i--) {
                            uint8_t nibble = (did >> (i * 4)) & 0xF;
                            hex_str[11-i] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
                        }
                        hex_str[11] = ')';
                        hex_str[12] = '\0';
                        
                        terminal_writestring(hex_str);
                        terminal_writestring("\n");
                        
                        pci_device_count++;
                    }
                    
                    // If this is function 0 and it's not a multi-function device,
                    // don't check the other functions
                    if (func == 0) {
                        uint8_t header_type = pci_config_read_byte(bus, slot, func, PCI_HEADER_TYPE);
                        if ((header_type & 0x80) == 0) {
                            break; // Not multi-function, skip other functions
                        }
                    }
                }
            }
        }
    }
    
    // Print summary
    terminal_writestring("PCI scan complete. Found ");
    char count_str[16];
    int count = pci_device_count;
    int pos = 0;
    
    if (count == 0) {
        count_str[pos++] = '0';
    } else {
        // Convert number to string
        char temp[16];
        int temp_pos = 0;
        while (count > 0) {
            temp[temp_pos++] = '0' + (count % 10);
            count /= 10;
        }
        // Reverse the string
        for (int i = temp_pos - 1; i >= 0; i--) {
            count_str[pos++] = temp[i];
        }
    }
    count_str[pos] = '\0';
    
    terminal_writestring(count_str);
    terminal_writestring(" devices.\n");
}

void pci_init(void) {
    // Initialize PCI subsystem and scan for devices
    pci_scan_bus();
}

// Function to get the list of found devices
int pci_get_device_count(void) {
    return pci_device_count;
}

pci_device_t* pci_get_device(int index) {
    if (index >= 0 && index < pci_device_count) {
        return &pci_devices[index];
    }
    return NULL;
}

// Find device function that uses the scanned devices
bool pci_find_device(uint16_t vendor_id, uint16_t device_id, pci_device_t* device) {
    for (int i = 0; i < pci_device_count; i++) {
        if (pci_devices[i].vendor_id == vendor_id && pci_devices[i].device_id == device_id) {
            if (device) {
                *device = pci_devices[i];
            }
            return true;
        }
    }
    return false;
}
