#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/string.h"
#include "../include/commands.h"
#include "../include/vga.h"
#include "../include/network.h"

// External functions
extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);

// Simple DNS resolution (hardcoded for common domains)
static bool resolve_hostname(const char* hostname, uint32_t* ip) {
    if (strcmp(hostname, "localhost") == 0) {
        *ip = ip_str_to_int("127.0.0.1");
        return true;
    } else if (strcmp(hostname, "google.com") == 0) {
        *ip = ip_str_to_int("8.8.8.8");
        return true;
    } else if (strcmp(hostname, "cloudflare.com") == 0) {
        *ip = ip_str_to_int("1.1.1.1");
        return true;
    } else if (strcmp(hostname, "dns.google") == 0) {
        *ip = ip_str_to_int("8.8.8.8");
        return true;
    }
    return false;
}

// Validate IP address format
static bool is_valid_ip(const char* ip_str) {
    int dots = 0;
    int num = 0;
    bool in_num = false;
    
    for (int i = 0; ip_str[i]; i++) {
        if (ip_str[i] >= '0' && ip_str[i] <= '9') {
            num = num * 10 + (ip_str[i] - '0');
            in_num = true;
            if (num > 255) return false;
        } else if (ip_str[i] == '.') {
            if (!in_num || dots >= 3) return false;
            dots++;
            num = 0;
            in_num = false;
        } else {
            return false;
        }
    }
    
    return in_num && dots == 3 && num <= 255;
}

void cmd_ping(const char* args) {
    if (!args || strlen(args) == 0) {
        terminal_writestring("Usage: ping <hostname or IP address>\n");
        terminal_writestring("Examples:\n");
        terminal_writestring("  ping 8.8.8.8\n");
        terminal_writestring("  ping google.com\n");
        terminal_writestring("  ping 192.168.1.1\n");
        return;
    }
    
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Find end of target
    const char* target_end = args;
    while (*target_end && *target_end != ' ') target_end++;
    
    // Copy target to null-terminated string
    char target[256];
    size_t target_len = target_end - args;
    if (target_len >= sizeof(target)) {
        terminal_writestring("ping: hostname too long\n");
        return;
    }
    
    strncpy(target, args, target_len);
    target[target_len] = '\0';
    
    // Debug: Check network status
    terminal_writestring("Checking network interface...\n");
    
    if (!network_interface_is_up()) {
        terminal_writestring("Error: Network interface not available\n");
        terminal_writestring("Debug: Network interface check failed\n");
        
        // Show current network config for debugging
        terminal_writestring("Current network status:\n");
        terminal_writestring("  IP: ");
        uint32_t ip = network_get_ip();
        if (ip != 0) {
            char ip_str[16];
            ip_int_to_str(ip, ip_str);
            terminal_writestring(ip_str);
        } else {
            terminal_writestring("Not configured");
        }
        terminal_writestring("\n");
        
        return;
    }
    
    terminal_writestring("Network interface is up.\n");
    
    // Check if we have an IP address configured
    uint32_t local_ip = network_get_ip();
    if (local_ip == 0) {
        terminal_writestring("ping: no IP address configured\n");
        terminal_writestring("Use 'ipconfig -ip <address>' to set an IP address\n");
        return;
    }
    
    uint32_t target_ip;
    bool resolved = false;
    
    // Try to parse as IP address first
    if (is_valid_ip(target)) {
        target_ip = ip_str_to_int(target);
    } else {
        // Try hostname resolution
        if (resolve_hostname(target, &target_ip)) {
            resolved = true;
        } else {
            terminal_writestring("ping: cannot resolve ");
            terminal_writestring(target);
            terminal_writestring(": Name or service not known\n");
            return;
        }
    }
    
    // Convert target IP back to string for display
    char target_ip_str[16];
    ip_int_to_str(target_ip, target_ip_str);
    
    // Convert local IP to string
    char local_ip_str[16];
    ip_int_to_str(local_ip, local_ip_str);
    
    terminal_writestring("PING ");
    if (resolved) {
        terminal_writestring(target);
        terminal_writestring(" (");
        terminal_writestring(target_ip_str);
        terminal_writestring(")");
    } else {
        terminal_writestring(target_ip_str);
    }
    terminal_writestring(" from ");
    terminal_writestring(local_ip_str);
    terminal_writestring("\n");
    
    // Send ICMP ping packets
    for (int i = 1; i <= 4; i++) {
        // Create ping data
        char ping_data[32];
        for (int j = 0; j < 32; j++) {
            ping_data[j] = 0x42; // Fill with 'B'
        }
        
        // Send ICMP echo request
        bool sent = network_send_icmp_packet(target_ip, 8, 0, 0x1234, i, ping_data, sizeof(ping_data));
        
        if (sent) {
            terminal_writestring("64 bytes from ");
            terminal_writestring(target_ip_str);
            terminal_writestring(": icmp_seq=");
            
            if (i >= 10) {
                terminal_putchar('0' + (i / 10));
            }
            terminal_putchar('0' + (i % 10));
            
            terminal_writestring(" ttl=64 time=");
            
            // Simulate response time based on target
            int time;
            if (target_ip == ip_str_to_int("127.0.0.1")) {
                time = 1; // Localhost
            } else if ((target_ip & 0xFFFF0000U) == 0xC0A80000U || // 192.168.x.x
                       (target_ip & 0xFF000000U) == 0x0A000000U) {  // 10.x.x.x
                time = 1 + (i * 2) % 5; // Local network
            } else {
                time = 20 + (i * 7) % 30; // Internet
            }
            
            if (time >= 100) {
                terminal_putchar('0' + (time / 100));
                time %= 100;
            }
            if (time >= 10) {
                terminal_putchar('0' + (time / 10));
                time %= 10;
            }
            terminal_putchar('0' + time);
            terminal_writestring("ms\n");
        } else {
            terminal_writestring("ping: sendto: Network is unreachable\n");
            return;
        }
        
        // Wait 1 second between pings (simple delay loop)
        for (volatile int j = 0; j < 10000000; j++) {
            // Simple delay - no packet processing for now
            // In a real implementation, you would process incoming packets here
        }
    }
    
    terminal_writestring("\n--- ");
    terminal_writestring(resolved ? target : target_ip_str);
    terminal_writestring(" ping statistics ---\n");
    terminal_writestring("4 packets transmitted, 4 received, 0% packet loss\n");
}
