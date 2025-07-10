#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/string.h"
#include "../include/keyboard.h"
#include "../include/commands.h"
#include "../include/filesystem.h"
#include "../include/vga.h"
#include "../include/io.h"
#include "../include/pci.h"
#include "../include/network.h"
#include "../include/dhcp.h"
#include "../include/kernel.h"

// Multiboot header definitions
#define MULTIBOOT_MAGIC 0x1BADB002
#define MULTIBOOT_FLAGS 0x00000003
#define MULTIBOOT_CHECKSUM -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

// Multiboot structure definition
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

// Declare the multiboot_info variable that will be used by system.c
multiboot_info_t* multiboot_info = NULL;

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

#define MAX_COMMAND_LENGTH 256
char current_command[MAX_COMMAND_LENGTH];
size_t command_length = 0;

void terminal_initialize(void) 
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) 
{
    terminal_color = color;
}

uint8_t terminal_getcolor(void) {
    return terminal_color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_scroll(void) {
    // Move all lines up by one
    for (size_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            size_t current_index = y * VGA_WIDTH + x;
            size_t next_index = (y + 1) * VGA_WIDTH + x;
            terminal_buffer[current_index] = terminal_buffer[next_index];
        }
    }
    
    // Clear the last line
    for (size_t x = 0; x < VGA_WIDTH; x++) {
        size_t index = (VGA_HEIGHT - 1) * VGA_WIDTH + x;
        terminal_buffer[index] = vga_entry(' ', terminal_color);
    }
    
    // Move cursor to the beginning of the last line
    terminal_row = VGA_HEIGHT - 1;
    terminal_column = 0;
    
    // Update the hardware cursor position
    update_cursor(terminal_row, terminal_column);
}

void terminal_backspace(void) {
    if (terminal_column > 0) {
        // Normal backspace within the same line
        terminal_column--;
        terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    } else if (terminal_row > 0) {
        // Backspace at beginning of line - go to end of previous line
        terminal_row--;
        
        // Find the last non-space character on the previous line
        terminal_column = VGA_WIDTH - 1;
        while (terminal_column > 0) {
            size_t index = terminal_row * VGA_WIDTH + terminal_column;
            uint16_t entry = terminal_buffer[index];
            char ch = (char)(entry & 0xFF);
            if (ch != ' ') {
                break;
            }
            terminal_column--;
        }
        
        // Clear the character at this position
        terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
    }
    update_cursor(terminal_row, terminal_column);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row >= VGA_HEIGHT) {
            terminal_scroll();
        }
        update_cursor(terminal_row, terminal_column);
        return;
    }
    
    if (c == '\b') {
        terminal_backspace();
        return;
    }

    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        terminal_row++;
        if (terminal_row >= VGA_HEIGHT) {
            terminal_scroll();
        }
    }
    update_cursor(terminal_row, terminal_column);
}

void terminal_write(const char* data, size_t size) 
{
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) 
{
    terminal_write(data, strlen(data));
}

void update_cursor(int row, int col) {
    unsigned short position = (row * VGA_WIDTH) + col;

    // Cursor Low Port (0x3D4)
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    // Cursor High Port (0x3D4)
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}

void print_prompt(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("admin@syncwideos");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring(":");
    
    // Get the current directory path
    terminal_setcolor(VGA_COLOR_CYAN);
    terminal_writestring(fs_getcwd());
    
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("$ ");
    update_cursor(terminal_row, terminal_column);
}

void process_command(const char* cmd) {
    // Skip leading spaces
    while (*cmd == ' ') cmd++;
    
    // Find the end of the command (first space or null terminator)
    const char* cmd_end = cmd;
    while (*cmd_end && *cmd_end != ' ') cmd_end++;
    
    size_t cmd_length = cmd_end - cmd;
    
    // Check for empty command
    if (cmd_length == 0) {
        print_prompt();
        return;
    }
    
    if (strncmp(cmd, "help", cmd_length) == 0 && cmd_length == 4) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_help(args);
        print_prompt();
        return;
    }

    // Check for "clear" command
    if (strncmp(cmd, "clear", cmd_length) == 0 && cmd_length == 5) {
        terminal_clear();
        print_prompt();
        return;
    }
    
    // Check for "echo" command
    if (strncmp(cmd, "echo", cmd_length) == 0 && cmd_length == 4) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_echo(args);
        print_prompt();
        return;
    }
    
    // Check for "system" command
    if (strncmp(cmd, "system", cmd_length) == 0 && cmd_length == 6) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_system(args);
        print_prompt();
        return;
    }
    
    // Filesystem commands
    if (strncmp(cmd, "ls", cmd_length) == 0 && cmd_length == 2) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_ls(args);
        print_prompt();
        return;
    }
    
    if (strncmp(cmd, "cd", cmd_length) == 0 && cmd_length == 2) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_cd(args);
        print_prompt();
        return;
    }
    
    if (strncmp(cmd, "mkdir", cmd_length) == 0 && cmd_length == 5) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_mkdir(args);
        print_prompt();
        return;
    }

    if (strncmp(cmd, "read", cmd_length) == 0 && cmd_length == 4) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_read(args);
        print_prompt();
        return;
    }

        // New file operation commands
    if (strncmp(cmd, "write", cmd_length) == 0 && cmd_length == 5) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_write(args);
        print_prompt();
        return;
    }

    if (strncmp(cmd, "touch", cmd_length) == 0 && cmd_length == 5) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_touch(args);
        print_prompt();
        return;
    }

    if (strncmp(cmd, "cp", cmd_length) == 0 && cmd_length == 2) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_cp(args);
        print_prompt();
        return;
    }

    if (strncmp(cmd, "rm", cmd_length) == 0 && cmd_length == 2) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_rm(args);
        print_prompt();
        return;
    }

    if (strncmp(cmd, "mv", cmd_length) == 0 && cmd_length == 2) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_mv(args);
        print_prompt();
        return;
    }

     if (strncmp(cmd, "tee", cmd_length) == 0 && cmd_length == 3) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_tee(args);
        print_prompt();
        return;
    }

    if (strncmp(cmd, "wc", cmd_length) == 0 && cmd_length == 2) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_wc(args);
        print_prompt();
        return;
    }

    // Alternative command for reading files
    if (strncmp(cmd, "cat", cmd_length) == 0 && cmd_length == 3) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_read(args); // Use the same function as read
        print_prompt();
        return;
    }

    // Print working directory
    if (strncmp(cmd, "pwd", cmd_length) == 0 && cmd_length == 3) {
        cmd_pwd();
        print_prompt();
        return;
    }

    // Check for "mount" command
    if (strncmp(cmd, "mount", cmd_length) == 0 && cmd_length == 5) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_mount(args);
        print_prompt();
        return;
    }

    // Check for "unmount" command
    if (strncmp(cmd, "unmount", cmd_length) == 0 && cmd_length == 7) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_unmount(args);
        print_prompt();
        return;
    }
    
    // Filesystem info command
    if (strncmp(cmd, "fsinfo", cmd_length) == 0 && cmd_length == 6) {
        if (!fs_is_mounted()) {
            terminal_writestring("Filesystem not mounted\n");
        } else {
            terminal_writestring("FAT32 Filesystem Information:\n");
            terminal_writestring("Status: Mounted\n");
            
            uint32_t total_space = fs_get_total_space();
            uint32_t free_space = fs_get_free_space();
            
            terminal_writestring("Total space: ");
            // Simple size display
            char size_str[32];
            uint32_t mb = total_space / (1024 * 1024);
            // Simple number to string
            int pos = 0;
            if (mb == 0) {
                size_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (mb > 0) {
                    temp[temp_pos++] = '0' + (mb % 10);
                    mb /= 10;
                }
                for (int i = temp_pos - 1; i >= 0; i--) {
                    size_str[pos++] = temp[i];
                }
            }
            size_str[pos] = '\0';
            terminal_writestring(size_str);
            terminal_writestring(" MB\n");
            
            terminal_writestring("Free space: ");
            mb = free_space / (1024 * 1024);
            pos = 0;
            if (mb == 0) {
                size_str[pos++] = '0';
            } else {
                char temp[16];
                int temp_pos = 0;
                while (mb > 0) {
                    temp[temp_pos++] = '0' + (mb % 10);
                    mb /= 10;
                }
                for (int i = temp_pos - 1; i >= 0; i--) {
                    size_str[pos++] = temp[i];
                }
            }
            size_str[pos] = '\0';
            terminal_writestring(size_str);
            terminal_writestring(" MB\n");
        }
        print_prompt();
        return;
    }

    // Text editor command
    if (strncmp(cmd, "pia", cmd_length) == 0 && cmd_length == 3) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_pia(args);
        print_prompt();
        return;
    }

    // Network commands
    if (strncmp(cmd, "ipconfig", cmd_length) == 0 && cmd_length == 8) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_ipconfig(args);
        print_prompt();
        return;
    }

    if (strncmp(cmd, "ping", cmd_length) == 0 && cmd_length == 4) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_ping(args);
        print_prompt();
        return;
    }

    if (strncmp(cmd, "dhcp", cmd_length) == 0 && cmd_length == 4) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_dhcp(args);
        print_prompt();
        return;
    }
    
    if (strncmp(cmd, "install", cmd_length) == 0 && cmd_length == 7) {
        const char* args = cmd_end;
        while (*args == ' ') args++;
        cmd_install(args);
        print_prompt();
        return;
    }

    if (strncmp(cmd, "netstat", cmd_length) == 0 && cmd_length == 7) {
        cmd_netstat("");
        print_prompt();
        return;
    }

    // Unknown command
    terminal_writestring("Unknown command: ");
    terminal_write(cmd, cmd_length);
    terminal_writestring("\n");
    print_prompt();
}

void wait(uint32_t milliseconds) {
    // Simple CPU cycle-based delay
    volatile uint32_t delay = milliseconds * 500000; // Adjust multiplier as needed
    
    for (volatile uint32_t i = 0; i < delay; i++) {
        // Burn CPU cycles
        __asm__ volatile ("nop");
    }
}

// Alternative shorter delays
void wait_short(void) {
    for (volatile uint32_t i = 0; i < 100000; i++) {
        __asm__ volatile ("nop");
    }
}

void wait_long(void) {
    for (volatile uint32_t i = 0; i < 1000000; i++) {
        __asm__ volatile ("nop");
    }
}

void kernel_main(uint32_t magic, multiboot_info_t* mbd) {
    /* Store the multiboot info pointer for use by system.c */
    multiboot_info = mbd;

    terminal_initialize();

    // Initialize memory system first
    terminal_writestring("Initializing Memory System...\n");
    memory_init();

    terminal_writestring("Initializing disk subsystem...\n");
    disk_init();

    // Initialize FAT32 filesystem
    terminal_writestring("Initializing FAT32 filesystem...\n");
    if (fs_init()) {
        terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
        terminal_writestring("FAT32 filesystem initialized and mounted successfully.\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    } else {
        terminal_setcolor(VGA_COLOR_BROWN);
        terminal_writestring("Failed to initialize FAT32 filesystem.\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    }
    
    // Initialize PCI (required for network card detection)
    terminal_writestring("Initializing PCI...\n");
    pci_init();
    
    // Initialize network
    terminal_writestring("Initializing network...\n");
    network_init();
    
    // Set default network configuration
    terminal_writestring("Configuring network...\n");
    network_set_ip(ip_str_to_int("192.168.1.100"));
    network_set_gateway(ip_str_to_int("192.168.1.1"));
    network_set_netmask(ip_str_to_int("255.255.255.0"));
    
    if (network_is_ready()) {
        terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
        terminal_writestring("Network initialized successfully.\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
                
        // Show network configuration
        terminal_writestring("IP Address: 192.168.1.100\n");
        terminal_writestring("Gateway: 192.168.1.1\n");
        terminal_writestring("Netmask: 255.255.255.0\n");
    } else {
        terminal_setcolor(VGA_COLOR_BROWN);
        terminal_writestring("Network initialization failed (no hardware detected).\n");
        terminal_writestring("Network commands will work in simulation mode.\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    }
    
    // Show filesystem status
    if (fs_is_mounted()) {
        terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
        terminal_writestring("FAT32 filesystem ready. Use 'ls' to list files.\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    } else {
        terminal_setcolor(VGA_COLOR_BROWN);
        terminal_writestring("FAT32 filesystem not available.\n");
        terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    }
    
    terminal_writestring("Type 'help' for available commands.\n\n");

    terminal_writestring("\n");
    terminal_writestring("Initializing Terminal...\n");

    wait(1000000);
    
    terminal_writestring("\n");
    
    terminal_clear();
    
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("------------------------------------\n");
    terminal_writestring("|      Welcome to SyncWide OS      |\n");
    terminal_writestring("|       https://os.syncwi.de       |\n");
    terminal_writestring("|                                  |\n");
    terminal_writestring("|         Version: 0.7.1b          |\n");
    terminal_writestring("------------------------------------\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("\n");
    
    // Print initial prompt
    print_prompt();
    
    // Initialize keyboard
    keyboard_init();
    
    // Initialize command buffer
    command_length = 0;
    current_command[0] = '\0';
    
    static uint32_t dhcp_tick_counter = 0;

    // Main input loop
    while (1) {
        // Poll keyboard for input
        keyboard_poll();
        
        // Process any available keys
        char key = keyboard_get_key();

        // Process network packets
        network_process_packets();
        
        // DHCP tick (approximately every second)
        dhcp_tick_counter++;
        if (dhcp_tick_counter >= 100000) { // Adjust this value based on your loop speed
            dhcp_tick();
            dhcp_tick_counter = 0;
        }
        
        if (key != 0) {
            // Handle special keys or control characters
            if (key == '\n') {
                // Enter key pressed - execute command
                terminal_writestring("\n");
                current_command[command_length] = '\0';
                process_command(current_command);
                
                // Reset command buffer
                command_length = 0;
                current_command[0] = '\0';
            }
            else if (key == '\b') {
                // Backspace key pressed
                if (command_length > 0) {
                    command_length--;
                    current_command[command_length] = '\0';
                    terminal_backspace();
                }
            }
            else {
                // Regular character input
                if (command_length < MAX_COMMAND_LENGTH - 1) {
                    current_command[command_length++] = key;
                    current_command[command_length] = '\0';
                    terminal_putchar(key);
                    update_cursor(terminal_row, terminal_column);
                }
            }
        }
    }
}
