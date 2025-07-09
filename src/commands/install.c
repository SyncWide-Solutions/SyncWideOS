#include "../include/commands.h"
#include "../include/installer.h"
#include "../include/string.h"

// External function declarations
extern void terminal_writestring(const char* data);

void cmd_install(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check for help
    if (strncmp(args, "help", 4) == 0 || strncmp(args, "--help", 6) == 0) {
        terminal_writestring("SyncWide OS Installer\n");
        terminal_writestring("Usage: install [options]\n\n");
        terminal_writestring("Options:\n");
        terminal_writestring("  help, --help    Show this help message\n");
        terminal_writestring("  start           Start the installation wizard\n\n");
        terminal_writestring("The installer will guide you through:\n");
        terminal_writestring("  - Disk selection and partitioning\n");
        terminal_writestring("  - User account creation\n");
        terminal_writestring("  - System configuration\n");
        terminal_writestring("  - GRUB bootloader installation\n");
        return;
    }
    
    // Check for start command
    if (strncmp(args, "start", 5) == 0 || *args == '\0') {
        terminal_writestring("Starting SyncWide OS installer...\n\n");
        installer_run();
        return;
    }
    
    // Unknown argument
    terminal_writestring("Unknown install command: ");
    terminal_writestring(args);
    terminal_writestring("\n");
    terminal_writestring("Use 'install help' for usage information.\n");
}