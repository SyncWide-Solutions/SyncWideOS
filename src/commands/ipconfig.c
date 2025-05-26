#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/string.h"
#include "../include/commands.h"
#include "../include/vga.h"
#include "../include/network.h"

typedef struct {
    char ip_address[16];
    char subnet_mask[16];
    char gateway[16];
    char dns_primary[16];
    char dns_secondary[16];
} network_config_t;

static network_config_t current_config = {
    .ip_address = "192.168.1.100",
    .subnet_mask = "255.255.255.0",
    .gateway = "192.168.1.1",
    .dns_primary = "1.1.1.1",
    .dns_secondary = "8.8.8.8"
};

// External functions from kernel
extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);

void print_usage(void) {
    terminal_writestring("Usage: ipconfig [options]\n");
    terminal_writestring("Options:\n");
    terminal_writestring("  (no args)     Show current network configuration\n");
    terminal_writestring("  -ip <addr>    Set IP address\n");
    terminal_writestring("  -mask <mask>  Set subnet mask\n");
    terminal_writestring("  -gw <addr>    Set gateway address\n");
    terminal_writestring("  -dns1 <addr>  Set primary DNS server\n");
    terminal_writestring("  -dns2 <addr>  Set secondary DNS server\n");
    terminal_writestring("  -reset        Reset to default configuration\n");
    terminal_writestring("  -help         Show this help message\n");
}

// Simple string to integer conversion
int str_to_int(const char* str) {
    int result = 0;
    int sign = 1;
    
    if (*str == '-') {
        sign = -1;
        str++;
    }
    
    while (*str >= '0' && *str <= '9') {
        result = result * 10 + (*str - '0');
        str++;
    }
    
    return result * sign;
}

int validate_ip(const char* ip) {
    int current_num = 0;
    int num_count = 0;
    bool in_number = false;
    
    for (int i = 0; ip[i] != '\0'; i++) {
        if (ip[i] >= '0' && ip[i] <= '9') {
            current_num = current_num * 10 + (ip[i] - '0');
            in_number = true;
            if (current_num > 255) return 0; // Number too large
        } else if (ip[i] == '.') {
            if (!in_number || num_count >= 3) return 0; // Invalid format
            if (current_num > 255) return 0; // Number out of range
            num_count++;
            current_num = 0;
            in_number = false;
        } else {
            return 0; // Invalid character
        }
    }
    
    // Check the last number
    if (!in_number || num_count != 3) return 0;
    if (current_num > 255) return 0;
    
    return 1; // Valid IP address
}

void show_config(void) {
    terminal_writestring("SyncWideOS Network Configuration\n");
    terminal_writestring("================================\n");
    terminal_writestring("IP Address:      ");
    terminal_writestring(current_config.ip_address);
    terminal_writestring("\n");
    terminal_writestring("Subnet Mask:     ");
    terminal_writestring(current_config.subnet_mask);
    terminal_writestring("\n");
    terminal_writestring("Default Gateway: ");
    terminal_writestring(current_config.gateway);
    terminal_writestring("\n");
    terminal_writestring("Primary DNS:     ");
    terminal_writestring(current_config.dns_primary);
    terminal_writestring("\n");
    terminal_writestring("Secondary DNS:   ");
    terminal_writestring(current_config.dns_secondary);
    terminal_writestring("\n\n");
}

void reset_config(void) {
    strcpy(current_config.ip_address, "192.168.1.100");
    strcpy(current_config.subnet_mask, "255.255.255.0");
    strcpy(current_config.gateway, "192.168.1.1");
    strcpy(current_config.dns_primary, "1.1.1.1");
    strcpy(current_config.dns_secondary, "8.8.8.8");
    terminal_writestring("Network configuration reset to defaults.\n");
}

// Apply network settings to the network stack
static void apply_network_settings(void) {
    uint32_t ip = ip_str_to_int(current_config.ip_address);
    uint32_t gateway = ip_str_to_int(current_config.gateway);
    uint32_t netmask = ip_str_to_int(current_config.subnet_mask);
    
    network_set_ip(ip);
    network_set_gateway(gateway);
    network_set_netmask(netmask);
    
}

int apply_network_config(void) {
    terminal_writestring("Applying network configuration...\n");
    terminal_writestring("Configuring interface eth0:\n");
    terminal_writestring("  Setting IP: ");
    terminal_writestring(current_config.ip_address);
    terminal_writestring("/");
    terminal_writestring(current_config.subnet_mask);
    terminal_writestring("\n");
    terminal_writestring("  Setting gateway: ");
    terminal_writestring(current_config.gateway);
    terminal_writestring("\n");
    terminal_writestring("  Setting DNS servers: ");
    terminal_writestring(current_config.dns_primary);
    terminal_writestring(", ");
    terminal_writestring(current_config.dns_secondary);
    terminal_writestring("\n");
    
    // Apply settings to network stack
    apply_network_settings();
    
    terminal_writestring("Network configuration applied successfully.\n");
    return 0;
}

// Simple argument parser
typedef struct {
    char** args;
    int count;
    char buffer[256];
} arg_parser_t;

int parse_arguments(const char* input, arg_parser_t* parser) {
    if (!input || strlen(input) == 0) {
        parser->count = 0;
        return 0;
    }
    
    // Copy input to buffer
    strcpy(parser->buffer, input);
    
    // Count arguments
    parser->count = 0;
    bool in_arg = false;
    
    for (int i = 0; parser->buffer[i]; i++) {
        if (parser->buffer[i] != ' ' && !in_arg) {
            parser->count++;
            in_arg = true;
        } else if (parser->buffer[i] == ' ') {
            in_arg = false;
        }
    }
    
    if (parser->count == 0) return 0;
    
    // Allocate argument array (static allocation for simplicity)
    static char* static_args[16];
    parser->args = static_args;
    
    // Split arguments
    int arg_index = 0;
    in_arg = false;
    
    for (int i = 0; parser->buffer[i]; i++) {
        if (parser->buffer[i] != ' ' && !in_arg) {
            parser->args[arg_index++] = &parser->buffer[i];
            in_arg = true;
        } else if (parser->buffer[i] == ' ') {
            parser->buffer[i] = '\0'; // Null-terminate the argument
            in_arg = false;
        }
    }
    
    return parser->count;
}

// Implementation matching the header declaration
int ipconfig_main(int argc, char* argv[]) {
    if (argc <= 1) {
        show_config();
        return 0;
    }
    
    int config_changed = 0;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-help") == 0) {
            print_usage();
            return 0;
        }
        else if (strcmp(argv[i], "-reset") == 0) {
            reset_config();
            config_changed = 1;
        }
        else if (strcmp(argv[i], "-ip") == 0) {
            if (i + 1 >= argc) {
                terminal_writestring("Error: -ip requires an IP address\n");
                return 1;
            }
            if (!validate_ip(argv[i + 1])) {
                terminal_writestring("Error: Invalid IP address format\n");
                return 1;
            }
            strcpy(current_config.ip_address, argv[i + 1]);
            terminal_writestring("IP address set to: ");
            terminal_writestring(argv[i + 1]);
            terminal_writestring("\n");
            config_changed = 1;
            i++;
        }
        else if (strcmp(argv[i], "-mask") == 0) {
            if (i + 1 >= argc) {
                terminal_writestring("Error: -mask requires a subnet mask\n");
                return 1;
            }
            if (!validate_ip(argv[i + 1])) {
                terminal_writestring("Error: Invalid subnet mask format\n");
                return 1;
            }
            strcpy(current_config.subnet_mask, argv[i + 1]);
            terminal_writestring("Subnet mask set to: ");
            terminal_writestring(argv[i + 1]);
            terminal_writestring("\n");
            config_changed = 1;
            i++;
        }
        else if (strcmp(argv[i], "-gw") == 0) {
            if (i + 1 >= argc) {
                terminal_writestring("Error: -gw requires a gateway address\n");
                return 1;
            }
            if (!validate_ip(argv[i + 1])) {
                terminal_writestring("Error: Invalid gateway address format\n");
                return 1;
            }
            strcpy(current_config.gateway, argv[i + 1]);
            terminal_writestring("Gateway set to: ");
            terminal_writestring(argv[i + 1]);
            terminal_writestring("\n");
            config_changed = 1;
            i++;
        }
        else if (strcmp(argv[i], "-dns1") == 0) {
            if (i + 1 >= argc) {
                terminal_writestring("Error: -dns1 requires a DNS server address\n");
                return 1;
            }
            if (!validate_ip(argv[i + 1])) {
                terminal_writestring("Error: Invalid DNS server address format\n");
                return 1;
            }
            strcpy(current_config.dns_primary, argv[i + 1]);
            terminal_writestring("Primary DNS set to: ");
            terminal_writestring(argv[i + 1]);
            terminal_writestring("\n");
            config_changed = 1;
            i++;
        }
        else if (strcmp(argv[i], "-dns2") == 0) {
            if (i + 1 >= argc) {
                terminal_writestring("Error: -dns2 requires a DNS server address\n");
                return 1;
            }
            if (!validate_ip(argv[i + 1])) {
                terminal_writestring("Error: Invalid DNS server address format\n");
                return 1;
            }
            strcpy(current_config.dns_secondary, argv[i + 1]);
            terminal_writestring("Secondary DNS set to: ");
            terminal_writestring(argv[i + 1]);
            terminal_writestring("\n");
            config_changed = 1;
            i++;
        }
        else {
            terminal_writestring("Error: Unknown option '");
            terminal_writestring(argv[i]);
            terminal_writestring("'\n");
            print_usage();
            return 1;
        }
    }
    
    if (config_changed) {
        apply_network_config();
        terminal_writestring("\nUpdated configuration:\n");
        show_config();
    }
    
    return 0;
}

void cmd_ipconfig(const char* args) {
    if (!args || strlen(args) == 0) {
        // Call with no arguments
        char* argv[] = {"ipconfig"};
        ipconfig_main(1, argv);
        return;
    }
    
    // Parse arguments
    arg_parser_t parser;
    int argc = parse_arguments(args, &parser);
    
    if (argc > 0) {
        // Create full argv with program name
        static char* full_argv[17]; // Static allocation for 16 args + program name
        full_argv[0] = "ipconfig";
        
        for (int i = 0; i < argc && i < 16; i++) {
            full_argv[i + 1] = parser.args[i];
        }
        
        // Call main function
        ipconfig_main(argc + 1, full_argv);
    } else {
        // No arguments, show config
        char* argv[] = {"ipconfig"};
        ipconfig_main(1, argv);
    }
}

// Network configuration accessors
const char* get_current_ip(void) {
    return current_config.ip_address;
}

const char* get_current_gateway(void) {
    return current_config.gateway;
}

const char* get_current_dns_primary(void) {
    return current_config.dns_primary;
}
