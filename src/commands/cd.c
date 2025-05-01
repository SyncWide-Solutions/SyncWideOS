#include "../include/filesystem.h"

extern void terminal_writestring(const char* data);

void cmd_cd(const char* args) {
    if (!args || !*args) {
        // cd with no args goes to root
        fs_set_cwd(fs_get_root());
        return;
    }
    
    fs_node_t* dir = fs_find_node_by_path(args);
    
    if (!dir) {
        terminal_writestring("cd: cannot access '");
        terminal_writestring(args);
        terminal_writestring("': No such directory\n");
        return;
    }
    
    if (dir->type != FS_TYPE_DIRECTORY) {
        terminal_writestring("cd: '");
        terminal_writestring(args);
        terminal_writestring("' is not a directory\n");
        return;
    }
    
    fs_set_cwd(dir);
}
