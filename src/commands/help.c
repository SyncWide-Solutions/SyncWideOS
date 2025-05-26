#include "../include/commands.h"
#include "../include/string.h"  // Add this include
#include "../include/vga.h"     // Add this include

extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);

void cmd_help(const char* args) {
    // Skip leading spaces
    while (args && *args == ' ') args++;
    
    if (args && *args) {
        // Help for specific command
        if (strcmp(args, "echo") == 0) {
            terminal_writestring("echo - Display text\n");
            terminal_writestring("Usage: echo <text>\n");
            terminal_writestring("Example: echo Hello World\n");
        }
        else if (strcmp(args, "clear") == 0) {
            terminal_writestring("clear - Clear the terminal screen\n");
            terminal_writestring("Usage: clear\n");
        }
        else if (strcmp(args, "system") == 0) {
            terminal_writestring("system - Display system information\n");
            terminal_writestring("Usage: system [info|memory|cpu]\n");
        }
        else if (strcmp(args, "ls") == 0) {
            terminal_writestring("ls - List directory contents\n");
            terminal_writestring("Usage: ls [directory]\n");
            terminal_writestring("Example: ls /\n");
        }
        else if (strcmp(args, "cd") == 0) {
            terminal_writestring("cd - Change directory\n");
            terminal_writestring("Usage: cd <directory>\n");
            terminal_writestring("Example: cd /home\n");
        }
        else if (strcmp(args, "mkdir") == 0) {
            terminal_writestring("mkdir - Create directory\n");
            terminal_writestring("Usage: mkdir <directory_name>\n");
            terminal_writestring("Example: mkdir test_dir\n");
        }
        else if (strcmp(args, "read") == 0 || strcmp(args, "cat") == 0) {
            terminal_writestring("read/cat - Display file contents\n");
            terminal_writestring("Usage: read <filename>\n");
            terminal_writestring("Example: read test.txt\n");
        }
        else if (strcmp(args, "pwd") == 0) {
            terminal_writestring("pwd - Print working directory\n");
            terminal_writestring("Usage: pwd\n");
        }
        else if (strcmp(args, "mount") == 0) {
            terminal_writestring("mount - Mount FAT32 filesystem\n");
            terminal_writestring("Usage: mount\n");
        }
        else if (strcmp(args, "umount") == 0) {
            terminal_writestring("umount - Unmount filesystem\n");
            terminal_writestring("Usage: umount\n");
        }
        else if (strcmp(args, "fsinfo") == 0) {
            terminal_writestring("fsinfo - Display filesystem information\n");
            terminal_writestring("Usage: fsinfo\n");
        }
        else if (strcmp(args, "pia") == 0) {
            terminal_writestring("pia - Text editor\n");
            terminal_writestring("Usage: pia [filename]\n");
            terminal_writestring("Controls: Ctrl+S (save), Ctrl+Q (quit), F1 (help)\n");
        }
        else if (strcmp(args, "ipconfig") == 0) {
            terminal_writestring("ipconfig - Network configuration\n");
            terminal_writestring("Usage: ipconfig [show|set]\n");
        }
        else if (strcmp(args, "ping") == 0) {
            terminal_writestring("ping - Send ICMP ping\n");
            terminal_writestring("Usage: ping <ip_address>\n");
            terminal_writestring("Example: ping 192.168.1.1\n");
        }
        else if (strcmp(args, "dhcp") == 0) {
            terminal_writestring("dhcp - DHCP client control\n");
            terminal_writestring("Usage: dhcp [start|stop|status]\n");
        }
        else if (strcmp(args, "netstat") == 0) {
            terminal_writestring("netstat - Display network status\n");
            terminal_writestring("Usage: netstat\n");
        }
        else {
            terminal_writestring("Unknown command: ");
            terminal_writestring(args);
            terminal_writestring("\n");
            terminal_writestring("Type 'help' for a list of available commands.\n");
        }
    } else {
        // General help
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        terminal_writestring("SyncWide OS - Available Commands:\n\n");
        terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
        
        terminal_writestring("System Commands:\n");
        terminal_writestring("  help [command]  - Show help information\n");
        terminal_writestring("  clear           - Clear the screen\n");
        terminal_writestring("  echo <text>     - Display text\n");
        terminal_writestring("  system [info]   - Show system information\n\n");
        
        terminal_writestring("Filesystem Commands:\n");
        terminal_writestring("  ls [path]       - List directory contents\n");
        terminal_writestring("  cd <path>       - Change directory\n");
        terminal_writestring("  pwd             - Print working directory\n");
        terminal_writestring("  mkdir <name>    - Create directory\n");
        terminal_writestring("  read <file>     - Display file contents\n");
        terminal_writestring("  cat <file>      - Display file contents (alias for read)\n");
        terminal_writestring("  mount           - Mount FAT32 filesystem\n");
        terminal_writestring("  umount          - Unmount filesystem\n");
        terminal_writestring("  fsinfo          - Show filesystem information\n\n");
        
        terminal_writestring("Text Editor:\n");
        terminal_writestring("  pia [file]      - Open text editor\n\n");
        
        terminal_writestring("Network Commands:\n");
        terminal_writestring("  ipconfig        - Network configuration\n");
        terminal_writestring("  ping <ip>       - Send ICMP ping\n");
        terminal_writestring("  dhcp [cmd]      - DHCP client control\n");
        terminal_writestring("  netstat         - Show network status\n\n");
        
        terminal_writestring("Use 'help <command>' for detailed information about a command.\n");
    }
}
