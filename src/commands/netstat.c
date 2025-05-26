#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/string.h"
#include "../include/commands.h"
#include "../include/network.h"
#include "../include/dhcp.h"

extern void terminal_writestring(const char* data);

void cmd_netstat(const char* args) {
    (void)args; // Suppress unused parameter warning
    
    terminal_writestring("Network Status\n");
    terminal_writestring("==============\n");
    
    // Check if network is ready
    if (network_is_ready()) {
        terminal_writestring("Network Interface: Ready\n");
        
        // Show MAC address
        uint8_t mac[6];
        network_get_mac_address(mac);
        terminal_writestring("MAC Address: ");
        for (int i = 0; i < 6; i++) {
            char hex[3];
            uint8_t byte = mac[i];
            hex[0] = (byte >> 4) < 10 ? '0' + (byte >> 4) : 'A' + (byte >> 4) - 10;
            hex[1] = (byte & 0xF) < 10 ? '0' + (byte & 0xF) : 'A' + (byte & 0xF) - 10;
            hex[2] = '\0';
            terminal_writestring(hex);
            if (i < 5) terminal_writestring(":");
        }
        terminal_writestring("\n");
        
        // Show IP configuration
        uint32_t ip = network_get_ip();
        if (ip != 0) {
            char ip_str[16];
            ip_int_to_str(ip, ip_str);
            terminal_writestring("IP Address: ");
            terminal_writestring(ip_str);
            terminal_writestring("\n");
            
            uint32_t gateway = network_get_gateway();
            if (gateway != 0) {
                ip_int_to_str(gateway, ip_str);
                terminal_writestring("Gateway: ");
                terminal_writestring(ip_str);
                terminal_writestring("\n");
            }
            
            uint32_t netmask = network_get_netmask();
            if (netmask != 0) {
                ip_int_to_str(netmask, ip_str);
                terminal_writestring("Netmask: ");
                terminal_writestring(ip_str);
                terminal_writestring("\n");
            }
        } else {
            terminal_writestring("IP Address: Not configured\n");
        }
        
        // Show DHCP status
        if (dhcp_is_active()) {
            terminal_writestring("DHCP Client: Active (");
            terminal_writestring(dhcp_get_state_name());
            terminal_writestring(")\n");
        } else {
            terminal_writestring("DHCP Client: Inactive\n");
        }
        
        // Show network statistics
        uint32_t p_sent, p_received, b_sent, b_received;
        network_get_stats(&p_sent, &p_received, &b_sent, &b_received);
        
        terminal_writestring("Packets Sent: ");
        // Simple number to string conversion
        char num_str[16];
        uint32_t num = p_sent;
        int pos = 0;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (num > 0) {
                temp[temp_pos++] = '0' + (num % 10);
                num /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                num_str[pos++] = temp[i];
            }
        }
        num_str[pos] = '\0';
        terminal_writestring(num_str);
        terminal_writestring("\n");
        
        terminal_writestring("Packets Received: ");
        num = p_received;
        pos = 0;
        if (num == 0) {
            num_str[pos++] = '0';
        } else {
            char temp[16];
            int temp_pos = 0;
            while (num > 0) {
                temp[temp_pos++] = '0' + (num % 10);
                num /= 10;
            }
            for (int i = temp_pos - 1; i >= 0; i--) {
                num_str[pos++] = temp[i];
            }
        }
        num_str[pos] = '\0';
        terminal_writestring(num_str);
        terminal_writestring("\n");
        
    } else {
        terminal_writestring("Network Interface: Not ready\n");
        terminal_writestring("Reason: No network hardware detected\n");
    }
}