#include "../include/filesystem.h"

extern void terminal_writestring(const char* data);

// External function from cd.c
extern const char* fs_get_current_directory(void);

void cmd_pwd(void) {
    if (!fs_is_mounted()) {
        terminal_writestring("pwd: filesystem not mounted\n");
        return;
    }
    
    terminal_writestring(fs_get_current_directory());
    terminal_writestring("\n");
}
