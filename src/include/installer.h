#ifndef INSTALLER_H
#define INSTALLER_H

#include <stdint.h>

// Color constants
#define COLOR_BLACK     0
#define COLOR_BLUE      1
#define COLOR_GREEN     2
#define COLOR_CYAN      3
#define COLOR_RED       4
#define COLOR_MAGENTA   5
#define COLOR_BROWN     6
#define COLOR_WHITE     7
#define COLOR_GRAY      8
#define COLOR_LIGHT_BLUE    9
#define COLOR_LIGHT_GREEN   10
#define COLOR_LIGHT_CYAN    11
#define COLOR_LIGHT_RED     12
#define COLOR_LIGHT_MAGENTA 13
#define COLOR_YELLOW        14
#define COLOR_BRIGHT_WHITE  15

// Installation steps
typedef enum {
    INSTALL_STEP_WELCOME,
    INSTALL_STEP_DISK_SELECTION,
    INSTALL_STEP_USER_CONFIG,
    INSTALL_STEP_CONFIRM,
    INSTALL_STEP_INSTALL,
    INSTALL_STEP_COMPLETE,
    INSTALL_STEP_ERROR
} installer_step_t;

// Installation configuration
typedef struct {
    char username[32];
    char password[64];
    char hostname[32];
    char target_disk[8];
} installer_config_t;

// Function declarations
void installer_init(void);
void installer_run(void);
void installer_welcome(void);
void installer_disk_selection(void);
void installer_user_config(void);
void installer_confirm(void);
void installer_complete(void);
void installer_error(void);
void installer_perform_installation(void);

// Installation functions
int installer_format_disk(const char* disk_index_str);
int installer_copy_system_files(void);
int installer_install_grub(const char* disk_index_str);
int installer_create_user_config(const installer_config_t* config);

// Utility functions
int atoi(const char* str);

#endif /* INSTALLER_H */
