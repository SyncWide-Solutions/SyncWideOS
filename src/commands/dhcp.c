#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/commands.h"
#include "../include/dhcp.h"
#include "../include/network.h"
#include "../include/rtl8139.h"
#include "../include/memory.h"

extern void terminal_writestring(const char* data);
extern uint32_t network_get_ip(void);
extern uint32_t network_get_gateway(void);
extern uint32_t network_get_netmask(void);
extern void ip_int_to_str(uint32_t ip, char* str);

void cmd_dhcp(const char* args) {
    if (!args || strlen(args) == 0) {
        terminal_writestring("Usage: dhcp [start|stop|status|info]\n");
        return;
    }
    
    if (strcmp(args, "start") == 0) {
        terminal_writestring("DHCP: Starting client...\n");
        dhcp_start();  // Don't check return value
        terminal_writestring("DHCP: Start command issued\n");
    }
    else if (strcmp(args, "stop") == 0) {
        terminal_writestring("DHCP: Stopping client...\n");
        dhcp_stop();
        terminal_writestring("DHCP: Client stopped\n");
    }
    else if (strcmp(args, "status") == 0) {
        terminal_writestring("DHCP Status:\n");
        terminal_writestring("State: ");
        terminal_writestring(dhcp_get_state_name());
        terminal_writestring("\n");
        
        if (dhcp_is_active()) {
            terminal_writestring("Status: Active\n");
        } else {
            terminal_writestring("Status: Inactive\n");
        }
    }
    else if (strcmp(args, "info") == 0) {
        terminal_writestring("DHCP Client Information:\n");
        terminal_writestring("State: ");
        terminal_writestring(dhcp_get_state_name());
        terminal_writestring("\n");
        
        if (dhcp_is_active()) {
            terminal_writestring("Status: Active\n");
        } else {
            terminal_writestring("Status: Inactive\n");
        }
        
        uint32_t current_ip = network_get_ip();
        if (current_ip != 0) {
            char ip_str[16];
            ip_int_to_str(current_ip, ip_str);
            terminal_writestring("Current IP: ");
            terminal_writestring(ip_str);
            terminal_writestring("\n");
        } else {
            terminal_writestring("Current IP: Not assigned\n");
        }
    }
    else {
        terminal_writestring("Unknown DHCP command. Use: start, stop, status, or info\n");
    }
}