#include "../include/io.h"
#include "../include/pci.h"
#include <stdint.h>

// External function declarations
extern void terminal_writestring(const char* data);
extern char* itoa(int value, char* str, int base);

// PCI configuration space access
#define PCI_CONFIG_ADDRESS  0xCF8
#define PCI_CONFIG_DATA     0xCFC

// Global variables
static int pci_device_count = 0;

// Create PCI configuration address
static uint32_t pci_config_address(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    return (1 << 31) | (bus << 16) | (device << 11) | (function << 8) | (offset & 0xFC);
}

// Read byte from PCI configuration space
uint8_t pci_config_read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inb(PCI_CONFIG_DATA + (offset & 3));
}

// Read word from PCI configuration space
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inw(PCI_CONFIG_DATA + (offset & 2));
}

// Read dword from PCI configuration space
uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    return inl(PCI_CONFIG_DATA);
}

// Write byte to PCI configuration space
void pci_config_write_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outb(PCI_CONFIG_DATA + (offset & 3), value);
}

// Write word to PCI configuration space
void pci_config_write_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outw(PCI_CONFIG_DATA + (offset & 2), value);
}

// Write dword to PCI configuration space
void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value) {
    uint32_t address = pci_config_address(bus, device, function, offset);
    outl(PCI_CONFIG_ADDRESS, address);
    outl(PCI_CONFIG_DATA, value);
}

// Get vendor name from vendor ID
static const char* pci_get_vendor_name(uint16_t vendor_id) {
    switch (vendor_id) {
        case 0x8086: return "Intel Corporation";
        case 0x1022: return "Advanced Micro Devices";
        case 0x10DE: return "NVIDIA Corporation";
        case 0x1002: return "ATI Technologies / AMD";
        case 0x15AD: return "VMware Inc.";
        case 0x80EE: return "Oracle VirtualBox";
        case 0x1234: return "QEMU";
        case 0x10EC: return "Realtek Semiconductor";
        case 0x8139: return "Realtek RTL8139";
        case 0x1013: return "Cirrus Logic";
        case 0x102B: return "Matrox Graphics";
        case 0x121A: return "3dfx Interactive";
        case 0x5333: return "S3 Graphics";
        case 0x9004: return "Adaptec";
        case 0x104C: return "Texas Instruments";
        case 0x1106: return "VIA Technologies";
        case 0x1039: return "Silicon Integrated Systems";
        case 0x10B7: return "3Com Corporation";
        case 0x14E4: return "Broadcom Corporation";
        case 0x168C: return "Qualcomm Atheros";
        default: return "Unknown Vendor";
    }
}

// Get class name from class code
static const char* pci_get_class_name(uint8_t class_code) {
    switch (class_code) {
        case 0x00: return "Unclassified";
        case 0x01: return "Mass Storage Controller";
        case 0x02: return "Network Controller";
        case 0x03: return "Display Controller";
        case 0x04: return "Multimedia Controller";
        case 0x05: return "Memory Controller";
        case 0x06: return "Bridge Device";
        case 0x07: return "Communication Controller";
        case 0x08: return "Generic System Peripheral";
        case 0x09: return "Input Device Controller";
        case 0x0A: return "Docking Station";
        case 0x0B: return "Processor";
        case 0x0C: return "Serial Bus Controller";
        case 0x0D: return "Wireless Controller";
        case 0x0E: return "Intelligent Controller";
        case 0x0F: return "Satellite Controller";
        case 0x10: return "Encryption Controller";
        case 0x11: return "Signal Processing Controller";
        case 0xFF: return "Unassigned Class";
        default: return "Unknown Class";
    }
}

// Check if a device exists at the given location
static int pci_device_exists(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor_id = pci_config_read_word(bus, device, function, PCI_VENDOR_ID);
    return (vendor_id != 0xFFFF);
}

// Check if device is multifunction
static int pci_is_multifunction(uint8_t bus, uint8_t device) {
    uint8_t header_type = pci_config_read_byte(bus, device, 0, PCI_HEADER_TYPE);
    return (header_type & 0x80) != 0;
}

// Scan a single PCI function
static void pci_scan_function(uint8_t bus, uint8_t device, uint8_t function) {
    uint16_t vendor_id = pci_config_read_word(bus, device, function, PCI_VENDOR_ID);
    uint16_t device_id = pci_config_read_word(bus, device, function, PCI_DEVICE_ID);
    uint8_t class_code = pci_config_read_byte(bus, device, function, PCI_CLASS_CODE);
    uint8_t subclass = pci_config_read_byte(bus, device, function, PCI_SUBCLASS);
    uint8_t prog_if = pci_config_read_byte(bus, device, function, PCI_PROG_IF);
    uint8_t revision = pci_config_read_byte(bus, device, function, PCI_REVISION_ID);
    
    char buffer[16];
    
    // Print device information
    terminal_writestring("  PCI ");
    itoa(bus, buffer, 16);
    terminal_writestring(buffer);
    terminal_writestring(":");
    itoa(device, buffer, 16);
    terminal_writestring(buffer);
    terminal_writestring(".");
    itoa(function, buffer, 16);
    terminal_writestring(buffer);
    terminal_writestring(" - ");
    
    terminal_writestring(pci_get_vendor_name(vendor_id));
    terminal_writestring(" (");
    itoa(vendor_id, buffer, 16);
    terminal_writestring("0x");
    terminal_writestring(buffer);
    terminal_writestring(":");
    itoa(device_id, buffer, 16);
    terminal_writestring("0x");
    terminal_writestring(buffer);
    terminal_writestring(") ");
    
    terminal_writestring(pci_get_class_name(class_code));
    terminal_writestring(" [");
    itoa(class_code, buffer, 16);
    terminal_writestring("0x");
    terminal_writestring(buffer);
    terminal_writestring(":");
    itoa(subclass, buffer, 16);
    terminal_writestring("0x");
    terminal_writestring(buffer);
    terminal_writestring(":");
    itoa(prog_if, buffer, 16);
    terminal_writestring("0x");
    terminal_writestring(buffer);
    terminal_writestring("] Rev ");
    itoa(revision, buffer, 16);
    terminal_writestring("0x");
    terminal_writestring(buffer);
    terminal_writestring("\n");
    
    pci_device_count++;
    
    // Check for PCI-to-PCI bridge
    if (class_code == PCI_CLASS_BRIDGE && subclass == 0x04) {
        // This is a PCI-to-PCI bridge, scan the secondary bus
        uint8_t secondary_bus = pci_config_read_byte(bus, device, function, 0x19);
        if (secondary_bus != 0) {
            terminal_writestring("    Scanning secondary bus ");
            itoa(secondary_bus, buffer, 10);
            terminal_writestring(buffer);
            terminal_writestring("\n");
            pci_scan_bus(secondary_bus);
        }
    }
}

// Scan a PCI device (all functions)
static void pci_scan_device(uint8_t bus, uint8_t device) {
    if (!pci_device_exists(bus, device, 0)) {
        return;
    }
    
    // Scan function 0
    pci_scan_function(bus, device, 0);
    
    // If multifunction device, scan other functions
    if (pci_is_multifunction(bus, device)) {
        for (uint8_t function = 1; function < 8; function++) {
            if (pci_device_exists(bus, device, function)) {
                pci_scan_function(bus, device, function);
            }
        }
    }
}

// Scan a PCI bus
void pci_scan_bus(uint8_t bus) {
    for (uint8_t device = 0; device < 32; device++) {
        pci_scan_device(bus, device);
    }
}

// Initialize PCI subsystem
void pci_init(void) {
    char buffer[16];
    
    terminal_writestring("Initializing PCI subsystem...\n");
    
    pci_device_count = 0;
    
    // Check if PCI is supported by trying to read from configuration space
    uint32_t test_address = pci_config_address(0, 0, 0, 0);
    outl(PCI_CONFIG_ADDRESS, test_address);
    uint32_t test_data = inl(PCI_CONFIG_ADDRESS);
    
    if (test_data != test_address) {
        terminal_writestring("PCI not supported or not present\n");
        return;
    }
    
    terminal_writestring("PCI bus scan starting...\n");
    
    // Check if this is a single PCI host controller or multiple
    uint8_t header_type = pci_config_read_byte(0, 0, 0, PCI_HEADER_TYPE);
    
    if ((header_type & 0x80) == 0) {
        // Single PCI host controller
        pci_scan_bus(0);
    } else {
        // Multiple PCI host controllers
        for (uint8_t function = 0; function < 8; function++) {
            if (pci_device_exists(0, 0, function)) {
                pci_scan_bus(function);
            }
        }
    }
    
    terminal_writestring("PCI scan complete. Found ");
    itoa(pci_device_count, buffer, 10);
    terminal_writestring(buffer);
    terminal_writestring(" devices.\n");
}

// Get the number of detected PCI devices
int pci_get_device_count(void) {
    return pci_device_count;
}
