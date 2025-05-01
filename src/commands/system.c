#include "../include/commands.h"
#include "../include/vga.h"
#include "../include/io.h"
#include <stddef.h>
#include <stdint.h>

// External function declarations
extern void terminal_writestring(const char* data);
extern size_t strlen(const char* str);

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
    outb(0xB004, 0x2000);
    
    // Try ACPI shutdown via Port 0x604
    outb(0x604, 0x2000);
    
    // Try APM (Advanced Power Management) shutdown
    outb(0x1004, 0x1234);
    
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

// System command handler
void cmd_system(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check for empty arguments
    if (*args == '\0') {
        terminal_writestring("Usage: system [restart|reboot|shutdown|powerdown]\n");
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
    
    // Unknown argument
    terminal_writestring("Unknown system command: ");
    terminal_writestring(args);
    terminal_writestring("\n");
    terminal_writestring("Available commands: restart, reboot, shutdown, powerdown\n");
}
