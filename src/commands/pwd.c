#include "../include/filesystem.h"
#include "../include/commands.h"

extern void terminal_writestring(const char* data);

void cmd_pwd(void) {
    if (!fs_is_mounted()) {
        terminal_writestring("pwd: FAT32 filesystem not mounted\n");
        return;
    }
    
    const char* cwd = fs_getcwd();
    terminal_writestring(cwd);
    terminal_writestring("\n");
}
