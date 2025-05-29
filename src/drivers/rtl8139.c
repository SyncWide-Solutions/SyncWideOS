#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/io.h"
#include "../include/pci.h"
#include "../include/rtl8139.h"
#include "../include/string.h"

// RTL8139 PCI IDs
#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

// RTL8139 registers
#define RTL8139_MAC0        0x00
#define RTL8139_MAR0        0x08
#define RTL8139_TSD0        0x10
#define RTL8139_TSAD0       0x20
#define RTL8139_RBSTART     0x30
#define RTL8139_ERBCR       0x34
#define RTL8139_ERSR        0x36
#define RTL8139_CR          0x37  // Command register (CHIPCMD)
#define RTL8139_CAPR        0x38
#define RTL8139_CBR         0x3A
#define RTL8139_IMR         0x3C
#define RTL8139_ISR         0x3E
#define RTL8139_TCR         0x40  // Transmit configuration (TXCONFIG)
#define RTL8139_RCR         0x44  // Receive configuration (RXCONFIG)
#define RTL8139_TCTR        0x48
#define RTL8139_MPC         0x4C
#define RTL8139_9346CR      0x50
#define RTL8139_CONFIG0     0x51
#define RTL8139_CONFIG1     0x52
#define RTL8139_MSR         0x58
#define RTL8139_MULINT      0x5C
#define RTL8139_BMCR        0x62
#define RTL8139_BMSR        0x64
#define RTL8139_ANAR        0x66
#define RTL8139_ANLPAR      0x68
#define RTL8139_ANER        0x6A
#define RTL8139_DIS         0x6C
#define RTL8139_FCSC        0x6E
#define RTL8139_NWAYTR      0x70
#define RTL8139_REC         0x72

// Command register bits
#define RTL8139_CR_RST      0x10  // Reset
#define RTL8139_CR_RE       0x08  // Receiver enable
#define RTL8139_CR_TE       0x04  // Transmitter enable

// Receive configuration register bits
#define RTL8139_RCR_WRAP    0x80
#define RTL8139_RCR_AB      0x01  // Accept broadcast
#define RTL8139_RCR_AM      0x02  // Accept multicast
#define RTL8139_RCR_APM     0x04  // Accept physical match
#define RTL8139_RCR_AAP     0x08  // Accept all packets

// Transmit configuration register bits
#define RTL8139_TCR_IFG96   (3 << 24)
#define RTL8139_TCR_MXDMA   (7 << 8)

// Interrupt bits
#define RTL8139_INT_RX_OK           0x0001
#define RTL8139_INT_RX_ERR          0x0002
#define RTL8139_INT_TX_OK           0x0004
#define RTL8139_INT_TX_ERR          0x0008
#define RTL8139_INT_RXBUF_OVERFLOW  0x0010

// Buffer sizes
#define RTL8139_RX_BUFFER_SIZE 8192
#define RTL8139_TX_BUFFER_SIZE 1536

// Device state
typedef struct {
    uint16_t io_base;
    uint8_t mac_address[6];
    uint8_t* rx_buffer;
    uint8_t* tx_buffer[4];
    uint16_t rx_buffer_pos;
    uint8_t tx_buffer_index;
    bool initialized;
} rtl8139_device_t;

static rtl8139_device_t rtl8139_dev = {0};

// PCI device location
static uint8_t rtl8139_bus = 0;
static uint8_t rtl8139_device_num = 0;
static uint8_t rtl8139_function = 0;

// Register access functions
static void rtl8139_write_reg8(uint16_t reg, uint8_t value) {
    outb(rtl8139_dev.io_base + reg, value);
}

static void rtl8139_write_reg16(uint16_t reg, uint16_t value) {
    outw(rtl8139_dev.io_base + reg, value);
}

static void rtl8139_write_reg32(uint16_t reg, uint32_t value) {
    outl(rtl8139_dev.io_base + reg, value);
}

static uint8_t rtl8139_read_reg8(uint16_t reg) {
    return inb(rtl8139_dev.io_base + reg);
}

static uint16_t rtl8139_read_reg16(uint16_t reg) {
    return inw(rtl8139_dev.io_base + reg);
}

static uint32_t rtl8139_read_reg32(uint16_t reg) {
    return inl(rtl8139_dev.io_base + reg);
}

static void rtl8139_reset(void) {
    rtl8139_write_reg8(RTL8139_CR, RTL8139_CR_RST);
    while (rtl8139_read_reg8(RTL8139_CR) & RTL8139_CR_RST) {
        // Wait for reset to complete
    }
}

static void rtl8139_read_mac_address(void) {
    for (int i = 0; i < 6; i++) {
        rtl8139_dev.mac_address[i] = rtl8139_read_reg8(RTL8139_MAC0 + i);
    }
}

static bool rtl8139_init_rx_buffer(void) {
    // Use static buffer for simplicity
    static uint8_t rx_buffer_static[RTL8139_RX_BUFFER_SIZE + 16];
    rtl8139_dev.rx_buffer = rx_buffer_static;
    rtl8139_dev.rx_buffer_pos = 0;
    
    // Set receive buffer address
    rtl8139_write_reg32(RTL8139_RBSTART, (uint32_t)rtl8139_dev.rx_buffer);
    
    return true;
}

static bool rtl8139_init_tx_buffers(void) {
    // Use static buffers for simplicity
    static uint8_t tx_buffer_static[4][RTL8139_TX_BUFFER_SIZE];
    
    for (int i = 0; i < 4; i++) {
        rtl8139_dev.tx_buffer[i] = tx_buffer_static[i];
        // Set transmit buffer address
        rtl8139_write_reg32(RTL8139_TSAD0 + (i * 4), (uint32_t)rtl8139_dev.tx_buffer[i]);
    }
    
    rtl8139_dev.tx_buffer_index = 0;
    return true;
}

static void rtl8139_configure(void) {
    // Enable receiver and transmitter
    rtl8139_write_reg8(RTL8139_CR, RTL8139_CR_RE | RTL8139_CR_TE);
    
    // Configure receive settings
    uint32_t rx_config = RTL8139_RCR_AB | RTL8139_RCR_AM | RTL8139_RCR_APM | RTL8139_RCR_AAP | RTL8139_RCR_WRAP;
    rtl8139_write_reg32(RTL8139_RCR, rx_config);
    
    // Configure transmit settings
    uint32_t tx_config = RTL8139_TCR_IFG96 | RTL8139_TCR_MXDMA;
    rtl8139_write_reg32(RTL8139_TCR, tx_config);
    
    // Enable interrupts
    uint16_t imr = RTL8139_INT_RX_OK | RTL8139_INT_TX_OK |
                   RTL8139_INT_RX_ERR | RTL8139_INT_TX_ERR |
                   RTL8139_INT_RXBUF_OVERFLOW;
    rtl8139_write_reg16(RTL8139_IMR, imr);
}

static bool rtl8139_find_device(void) {
    // Scan PCI bus for RTL8139 device
    uint8_t bus = 0;
    do {
        for (uint8_t device = 0; device < 32; device++) {
            for (uint8_t function = 0; function < 8; function++) {
                uint32_t config = pci_config_read_dword(bus, device, function, 0);
                uint16_t vendor_id = config & 0xFFFF;
                uint16_t device_id = (config >> 16) & 0xFFFF;
                
                if (vendor_id == RTL8139_VENDOR_ID && device_id == RTL8139_DEVICE_ID) {
                    rtl8139_bus = bus;
                    rtl8139_device_num = device;
                    rtl8139_function = function;
                    return true;
                }
            }
        }
        bus++;
    } while (bus != 0);
    
    return false;
}

bool rtl8139_init(void) {
    // Find RTL8139 device on PCI bus
    if (!rtl8139_find_device()) {
        return false;
    }
    
    // Get I/O base address from PCI configuration
    uint32_t bar0 = pci_config_read_dword(rtl8139_bus, rtl8139_device_num, rtl8139_function, PCI_BAR0);
    rtl8139_dev.io_base = bar0 & 0xFFFC; // Clear bottom 2 bits
    
    // Enable PCI bus mastering
    uint16_t command = pci_config_read_word(rtl8139_bus, rtl8139_device_num, rtl8139_function, PCI_COMMAND);
    command |= PCI_COMMAND_MASTER | PCI_COMMAND_IO;
    pci_config_write_word(rtl8139_bus, rtl8139_device_num, rtl8139_function, PCI_COMMAND, command);
    
    // Reset the device
    rtl8139_reset();
    
    // Initialize buffers
    if (!rtl8139_init_rx_buffer() || !rtl8139_init_tx_buffers()) {
        return false;
    }
    
    // Read MAC address
    rtl8139_read_mac_address();
    
    // Configure the device
    rtl8139_configure();
    
    rtl8139_dev.initialized = true;
    return true;
}

static bool rtl8139_initialized = false;

bool rtl8139_is_initialized(void) {
    return rtl8139_initialized;
}


void rtl8139_get_mac_address(uint8_t* mac) {
    if (mac && rtl8139_dev.initialized) {
        for (int i = 0; i < 6; i++) {
            mac[i] = rtl8139_dev.mac_address[i];
        }
    }
}

bool rtl8139_send_packet(const uint8_t* data, uint16_t length) {
    if (!rtl8139_dev.initialized || length > RTL8139_TX_BUFFER_SIZE) {
        return false;
    }
    
    // Copy data to transmit buffer
    for (uint16_t i = 0; i < length; i++) {
        rtl8139_dev.tx_buffer[rtl8139_dev.tx_buffer_index][i] = data[i];
    }
    
    // Start transmission
    uint16_t tx_status_reg = RTL8139_TSD0 + (rtl8139_dev.tx_buffer_index * 4);
    rtl8139_write_reg32(tx_status_reg, length);
    
    // Move to next transmit buffer
    rtl8139_dev.tx_buffer_index = (rtl8139_dev.tx_buffer_index + 1) % 4;
    
    return true;
}

int rtl8139_receive_packet(uint8_t* buffer, uint16_t max_length) {
    if (!rtl8139_dev.initialized) {
        return -1;
    }
    
    // Check if there's data available
    uint16_t current_pos = rtl8139_read_reg16(RTL8139_CBR);
    if (current_pos == rtl8139_dev.rx_buffer_pos) {
        return 0; // No data available
    }
    
    // Read packet header
    uint16_t* header = (uint16_t*)(rtl8139_dev.rx_buffer + rtl8139_dev.rx_buffer_pos);
    uint16_t packet_length = header[1] - 4; // Subtract CRC
    
    if (packet_length > max_length) {
        return -1; // Buffer too small
    }
    
    // Copy packet data
    uint8_t* packet_data = rtl8139_dev.rx_buffer + rtl8139_dev.rx_buffer_pos + 4;
    for (uint16_t i = 0; i < packet_length; i++) {
        buffer[i] = packet_data[i];
    }
    
    // Update buffer position
    rtl8139_dev.rx_buffer_pos = (rtl8139_dev.rx_buffer_pos + packet_length + 4 + 3) & ~3; // Align to 4 bytes
    rtl8139_dev.rx_buffer_pos %= RTL8139_RX_BUFFER_SIZE;
    
    // Update CAPR register
    rtl8139_write_reg16(RTL8139_CAPR, rtl8139_dev.rx_buffer_pos - 16);
    
    return packet_length;
}

void rtl8139_interrupt_handler(void) {
    if (!rtl8139_dev.initialized) {
        return;
    }
    
    uint16_t isr = rtl8139_read_reg16(RTL8139_ISR);
    
    if (isr & RTL8139_INT_RX_OK) {
        // Packet received
    }
    
    if (isr & RTL8139_INT_TX_OK) {
        // Packet transmitted
    }
    
    if (isr & RTL8139_INT_RX_ERR) {
        // Receive error
    }
    
    if (isr & RTL8139_INT_TX_ERR) {
        // Transmit error
    }
    
        if (isr & RTL8139_INT_RXBUF_OVERFLOW) {
        // Receive buffer overflow
    }
    
    // Clear interrupts
    rtl8139_write_reg16(RTL8139_ISR, isr);
}
