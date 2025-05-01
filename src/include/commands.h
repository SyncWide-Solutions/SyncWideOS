#ifndef COMMANDS_H
#define COMMANDS_H

// Existing command declarations
void cmd_echo(const char* args);
void cmd_system(const char* args);
void terminal_clear(void);

// New filesystem command declarations
void cmd_ls(const char* args);
void cmd_cd(const char* args);
void cmd_mkdir(const char* args);
void cmd_touch(const char* args);
void cmd_cat(const char* args);
void cmd_pwd(void);
void cmd_read(const char* args);
void cmd_help(const char* args);

#endif /* COMMANDS_H */
