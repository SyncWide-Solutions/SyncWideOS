#ifndef COMMANDS_H
#define COMMANDS_H

// System commands
void cmd_help(const char* args);
void cmd_echo(const char* args);
void cmd_clear(void);
void cmd_system(const char* args);

// File system commands
void cmd_ls(const char* args);
void cmd_cd(const char* args);
void cmd_mkdir(const char* args);
void cmd_read(const char* args);
void cmd_pwd(void);
void cmd_mount(const char* args);
void cmd_unmount(const char* args);

// File operation commands
void cmd_write(const char* args);
void cmd_touch(const char* args);
void cmd_cp(const char* args);
void cmd_rm(const char* args);
void cmd_mv(const char* args);
void cmd_tee(const char* args);
void cmd_wc(const char* args);

// Text editor
void cmd_pia(const char* args);

// Network commands
void cmd_ipconfig(const char* args);
void cmd_ping(const char* args);
void cmd_dhcp(const char* args);
void cmd_install(const char* args);
void cmd_netstat(const char* args);

#endif /* COMMANDS_H */
