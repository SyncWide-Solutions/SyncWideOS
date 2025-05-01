#include "../include/string.h"
#include <stdint.h>

extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);

void cmd_help(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check if a specific command was requested
    if (*args != '\0') {
        if (strcmp(args, "help") == 0) {
            terminal_writestring("help - Display information about available commands\n");
            terminal_writestring("Usage: help [command]\n");
            terminal_writestring("  If [command] is specified, show detailed help for that command\n");
        }
        else if (strcmp(args, "clear") == 0) {
            terminal_writestring("clear - Clear the terminal screen\n");
            terminal_writestring("Usage: clear\n");
        }
        else if (strcmp(args, "echo") == 0) {
            terminal_writestring("echo - Display a line of text\n");
            terminal_writestring("Usage: echo [text]\n");
            terminal_writestring("  Displays the [text] on the terminal\n");
        }
        else if (strcmp(args, "system") == 0) {
            terminal_writestring("system - System control commands\n");
            terminal_writestring("Usage: system [command]\n");
            terminal_writestring("  Commands:\n");
            terminal_writestring("    info    - Display system information\n");
            terminal_writestring("    restart - Restart the system\n");
            terminal_writestring("    reboot  - Alias for restart\n");
            terminal_writestring("    shutdown - Shutdown the system\n");
        }
        else if (strcmp(args, "ls") == 0) {
            terminal_writestring("ls - List directory contents\n");
            terminal_writestring("Usage: ls [directory]\n");
            terminal_writestring("  If [directory] is not specified, list the current directory\n");
        }
        else if (strcmp(args, "cd") == 0) {
            terminal_writestring("cd - Change the current directory\n");
            terminal_writestring("Usage: cd [directory]\n");
            terminal_writestring("  If [directory] is not specified, change to the root directory\n");
        }
        else if (strcmp(args, "mkdir") == 0) {
            terminal_writestring("mkdir - Create a new directory\n");
            terminal_writestring("Usage: mkdir [directory]\n");
            terminal_writestring("  Creates a new directory with the specified name\n");
        }
        else if (strcmp(args, "read") == 0) {
            terminal_writestring("read - Display the contents of a file\n");
            terminal_writestring("Usage: read [file]\n");
            terminal_writestring("  Displays the contents of the specified file\n");
        }
        else {
            terminal_writestring("No help available for '");
            terminal_writestring(args);
            terminal_writestring("'\n");
        }
        return;
    }
    
    // Display general help
    terminal_writestring("SyncWide OS Help\n");
    terminal_writestring("Available commands:\n\n");
    
    terminal_writestring("  help    - Display this help message\n");
    terminal_writestring("  clear   - Clear the terminal screen\n");
    terminal_writestring("  echo    - Display a line of text\n");
    terminal_writestring("  system  - System control commands\n");
    terminal_writestring("  ls      - List directory contents\n");
    terminal_writestring("  cd      - Change the current directory\n");
    terminal_writestring("  mkdir   - Create a new directory\n");
    terminal_writestring("  read    - Display the contents of a file\n");
    
    terminal_writestring("\nFor more information on a specific command, type 'help [command]'\n");
}
