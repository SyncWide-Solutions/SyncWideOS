#ifndef GPU_H
#define GPU_H

#include <stdint.h>

// GPU vendor IDs
#define GPU_VENDOR_NVIDIA       0x10DE
#define GPU_VENDOR_AMD          0x1002
#define GPU_VENDOR_INTEL        0x8086
#define GPU_VENDOR_VMWARE       0x15AD
#define GPU_VENDOR_VIRTUALBOX   0x80EE
#define GPU_VENDOR_QEMU         0x1234
#define GPU_VENDOR_CIRRUS       0x1013
#define GPU_VENDOR_S3           0x5333
#define GPU_VENDOR_MATROX       0x102B
#define GPU_VENDOR_3DFX         0x121A
#define GPU_VENDOR_TRIDENT      0x1023
#define GPU_VENDOR_TSENG        0x100C
#define GPU_VENDOR_SIS          0x1039
#define GPU_VENDOR_VIA          0x1106

// GPU types
typedef enum {
    GPU_TYPE_UNKNOWN = 0,
    GPU_TYPE_VGA,
    GPU_TYPE_SVGA,
    GPU_TYPE_ACCELERATED,
    GPU_TYPE_INTEGRATED,
    GPU_TYPE_DISCRETE,
    GPU_TYPE_VIRTUAL
} gpu_type_t;

// Memory types
typedef enum {
    GPU_MEM_UNKNOWN = 0,
    GPU_MEM_SHARED,
    GPU_MEM_DEDICATED,
    GPU_MEM_UNIFIED
} gpu_memory_type_t;

// GPU capabilities flags
#define GPU_CAP_2D              (1 << 0)
#define GPU_CAP_3D              (1 << 1)
#define GPU_CAP_HARDWARE_ACCEL  (1 << 2)
#define GPU_CAP_SHADER          (1 << 3)
#define GPU_CAP_DIRECTX         (1 << 4)
#define GPU_CAP_OPENGL          (1 << 5)
#define GPU_CAP_VULKAN          (1 << 6)
#define GPU_CAP_COMPUTE         (1 << 7)
#define GPU_CAP_VIDEO_DECODE    (1 << 8)
#define GPU_CAP_VIDEO_ENCODE    (1 << 9)
#define GPU_CAP_MULTI_MONITOR   (1 << 10)
#define GPU_CAP_HDMI            (1 << 11)
#define GPU_CAP_DISPLAYPORT     (1 << 12)
#define GPU_CAP_VGA_COMPAT      (1 << 13)

// Display mode structure
typedef struct {
    uint16_t width;
    uint16_t height;
    uint8_t bpp;
    uint8_t refresh_rate;
    uint8_t supported;
} gpu_display_mode_t;

// GPU device structure
typedef struct {
    uint8_t present;
    uint8_t primary;
    uint16_t vendor_id;
    uint16_t device_id;
    uint8_t revision;
    uint8_t class_code;
    uint8_t subclass;
    uint8_t prog_if;
    uint8_t interrupt_line;
    uint8_t interrupt_pin;
    uint32_t base_address[6];
    
    char vendor_name[32];
    char device_name[64];
    char driver_version[16];
    
    gpu_type_t gpu_type;
    gpu_memory_type_t memory_type;
    uint32_t memory_size;  // in MB
    uint32_t capabilities;
    
    // Current display mode
    uint16_t current_width;
    uint16_t current_height;
    uint8_t current_bpp;
    uint8_t current_refresh;
    
    // Supported display modes
    gpu_display_mode_t supported_modes[16];
    uint8_t mode_count;
    
    // Performance monitoring (placeholder)
    uint32_t core_clock;
    uint32_t memory_clock;
    uint32_t temperature;
    uint32_t fan_speed;
    uint32_t power_usage;
} gpu_device_t;

// Function declarations
void gpu_init(void);
void gpu_detect_devices(void);
int gpu_get_device_count(void);
gpu_device_t* gpu_get_device_info(int device_index);
void gpu_set_display_mode(int device_index, uint16_t width, uint16_t height, uint8_t bpp);
void gpu_print_device_info(int device_index);

// Helper functions
const char* gpu_get_vendor_name(uint16_t vendor_id);
const char* gpu_get_device_name(uint16_t vendor_id, uint16_t device_id);
void gpu_detect_vga_modes(gpu_device_t* gpu);
uint32_t gpu_detect_memory_size(gpu_device_t* gpu);
uint32_t gpu_detect_capabilities(gpu_device_t* gpu);

// PCI helper functions (should be declared in pci.h, but included here for completeness)
uint8_t pci_config_read_byte(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint16_t pci_config_read_word(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);
uint32_t pci_config_read_dword(uint8_t bus, uint8_t device, uint8_t function, uint8_t offset);

#endif // GPU_H
