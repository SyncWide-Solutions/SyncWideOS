#ifndef PCI_H
#define PCI_H

#include <stdint.h>

// PCI Configuration Space Registers
#define PCI_VENDOR_ID           0x00
#define PCI_DEVICE_ID           0x02
#define PCI_COMMAND             0x04
#define PCI_STATUS              0x06
#define PCI_REVISION_ID         0x08
#define PCI_PROG_IF             0x09
#define PCI_SUBCLASS            0x0A
#define PCI_CLASS_CODE          0x0B
#define PCI_CACHE_LINE_SIZE     0x0C
#define PCI_LATENCY_TIMER       0x0D
#define PCI_HEADER_TYPE         0x0E
#define PCI_BIST                0x0F
#define PCI_BAR0                0x10
#define PCI_BAR1                0x14
#define PCI_BAR2                0x18
#define PCI_BAR3                0x1C
#define PCI_BAR4                0x20
#define PCI_BAR5                0x24
#define PCI_CARDBUS_CIS         0x28
#define PCI_SUBSYSTEM_VENDOR_ID 0x2C
#define PCI_SUBSYSTEM_ID        0x2E
#define PCI_EXPANSION_ROM       0x30
#define PCI_CAPABILITIES        0x34
#define PCI_INTERRUPT_LINE      0x3C
#define PCI_INTERRUPT_PIN       0x3D
#define PCI_MIN_GRANT           0x3E
#define PCI_MAX_LATENCY         0x3F

// PCI Command Register bits
#define PCI_COMMAND_IO          0x0001  // Enable I/O Space
#define PCI_COMMAND_MEMORY      0x0002  // Enable Memory Space
#define PCI_COMMAND_MASTER      0x0004  // Enable Bus Mastering
#define PCI_COMMAND_SPECIAL     0x0008  // Enable Special Cycles
#define PCI_COMMAND_INVALIDATE  0x0010  // Memory Write and Invalidate Enable
#define PCI_COMMAND_VGA_PALETTE 0x0020  // VGA Palette Snoop
#define PCI_COMMAND_PARITY      0x0040  // Parity Error Response
#define PCI_COMMAND_WAIT        0x0080  // Enable address/data stepping
#define PCI_COMMAND_SERR        0x0100  // Enable SERR
#define PCI_COMMAND_FAST_BACK   0x0200  // Enable back-to-back writes

// PCI Status Register bits
#define PCI_STATUS_CAP_LIST     0x0010  // Support Capability List
#define PCI_STATUS_66MHZ        0x0020  // Support 66 Mhz PCI 2.1 bus
#define PCI_STATUS_UDF          0x0040  // Support User Definable Features
#define PCI_STATUS_FAST_BACK    0x0080  // Accept fast-back to back
#define PCI_STATUS_PARITY       0x0100  // Detected parity error
#define PCI_STATUS_DEVSEL_MASK  0x0600  // DEVSEL timing
#define PCI_STATUS_DEVSEL_FAST  0x0000
#define PCI_STATUS_DEVSEL_MEDIUM 0x0200
#define PCI_STATUS_DEVSEL_SLOW  0x0400
#define PCI_STATUS_SIG_TARGET_ABORT 0x0800 // Set on target abort
#define PCI_STATUS_REC_TARGET_ABORT 0x1000 // Master ack of "
#define PCI_STATUS_REC_MASTER_ABORT 0x2000 // Set on master abort
#define PCI_STATUS_SIG_SYSTEM_ERROR 0x4000 // Set when we drive SERR
#define PCI_STATUS_DETECTED_PARITY  0x8000 // Set on parity error

// PCI Class Codes
#define PCI_CLASS_NOT_DEFINED           0x00
#define PCI_CLASS_MASS_STORAGE          0x01
#define PCI_CLASS_NETWORK               0x02
#define PCI_CLASS_DISPLAY               0x03
#define PCI_CLASS_MULTIMEDIA            0x04
#define PCI_CLASS_MEMORY                0x05
#define PCI_CLASS_BRIDGE                0x06
#define PCI_CLASS_COMMUNICATION         0x07
#define PCI_CLASS_SYSTEM                0x08
#define PCI_CLASS_INPUT                 0x09
#define PCI_CLASS_DOCKING               0x0A
#define PCI_CLASS_PROCESSOR             0x0B
#define PCI_CLASS_SERIAL                0x0C
#define PCI_CLASS_WIRELESS              0x0D
#define PCI_CLASS_INTELLIGENT_IO        0x0E
#define PCI_CLASS_SATELLITE             0x0F
#define PCI_CLASS_ENCRYPTION            0x10
#define PCI_CLASS_DATA_ACQUISITION      0x11
#define PCI_CLASS_UNDEFINED             0xFF

// Function declarations
void pci_init(void);
void pci_scan_bus(uint8_t bus);
int pci_get_device_count(void);
uint8_t pci_config_read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
void pci_config_write_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint8_t value);
void pci_config_write_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint16_t value);
void pci_config_write_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset, uint32_t value);

#endif // PCI_H
