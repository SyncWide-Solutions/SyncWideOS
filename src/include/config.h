#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define CONFIG_FILE_PATH "/etc/syncwide.conf"
#define MAX_CONFIG_LINE_LENGTH 256

typedef struct {
    char username[32];
    char hostname[32];
    char version[16];
    uint8_t first_boot;
    uint32_t install_date;
} system_config_t;

// Function declarations
int config_load(system_config_t* config);
int config_save(const system_config_t* config);
void config_set_defaults(system_config_t* config);

#endif // CONFIG_H