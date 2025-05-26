#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/dhcp.h"
#include "../include/network.h"
#include "../include/rtl8139.h"
#include "../include/string.h"

// Forward declarations for network functions we need
extern void network_get_mac_address(uint8_t* mac);
extern bool network_send_ethernet_frame(const uint8_t* dest_mac, uint16_t ethertype, const void* payload, uint16_t payload_len);
extern void network_set_ip(uint32_t ip);
extern void network_set_gateway(uint32_t gateway);
extern void network_set_netmask(uint32_t mask);
extern uint32_t network_get_ip(void);
extern uint16_t htons(uint16_t hostshort);
extern uint16_t ntohs(uint16_t netshort);
extern uint32_t htonl(uint32_t hostlong);
extern uint32_t ntohl(uint32_t netlong);
extern uint16_t network_checksum(const void* data, size_t length);
extern void ip_int_to_str(uint32_t ip, char* str);

// Network protocol constants
#define ETHERTYPE_IP    0x0800
#define IP_PROTOCOL_UDP 17

// DHCP client state
static dhcp_state_t dhcp_state = DHCP_STATE_INIT;
static uint32_t dhcp_xid = 0;
static uint32_t dhcp_server_ip = 0;
static uint32_t dhcp_offered_ip = 0;
static uint32_t dhcp_lease_time = 0;
static uint32_t dhcp_timer = 0;
static bool dhcp_active = false;
static uint32_t dhcp_subnet_mask = 0;
static uint32_t dhcp_router = 0;
static uint32_t dhcp_dns_server = 0;

// DHCP magic cookie
//static const uint8_t dhcp_magic_cookie[4] = {0x63, 0x82, 0x53, 0x63};

// External functions
extern void terminal_writestring(const char* data);
extern void wait(uint32_t milliseconds);

// Simple random number generator
static uint32_t dhcp_random_seed = 12345;

static uint32_t dhcp_random(void) {
    dhcp_random_seed = dhcp_random_seed * 1103515245 + 12345;
    return dhcp_random_seed;
}

// Helper function to add DHCP option
/*static uint8_t* dhcp_add_option(uint8_t* ptr, uint8_t option, uint8_t length, const uint8_t* data) {
    *ptr++ = option;
    *ptr++ = length;
    for (uint8_t i = 0; i < length; i++) {
        *ptr++ = data[i];
    }
    return ptr;
}

// Helper function to add simple option
static uint8_t* dhcp_add_simple_option(uint8_t* ptr, uint8_t option, uint8_t value) {
    *ptr++ = option;
    *ptr++ = 1;
    *ptr++ = value;
    return ptr;
}*/

// Create DHCP packet
void dhcp_create_packet(dhcp_packet_t* packet, uint8_t message_type) {
    if (!packet) {
        return;
    }
    
    // Clear the packet
    uint8_t* ptr = (uint8_t*)packet;
    for (size_t i = 0; i < sizeof(dhcp_packet_t); i++) {
        ptr[i] = 0;
    }
    
    // Fill basic DHCP fields
    packet->op = 1; // BOOTREQUEST
    packet->htype = 1; // Ethernet
    packet->hlen = 6; // MAC address length
    packet->hops = 0;
    packet->xid = htonl(dhcp_xid);
    packet->secs = 0;
    packet->flags = htons(0x8000); // Broadcast flag
    packet->ciaddr = 0; // Client IP (we don't have one yet)
    packet->yiaddr = 0; // Your IP (server will fill this)
    packet->siaddr = 0; // Server IP
    packet->giaddr = 0; // Gateway IP
    
    // Set client MAC address
    network_get_mac_address(packet->chaddr);
    
    // Clear server name and boot file
    for (int i = 0; i < 64; i++) {
        packet->sname[i] = 0;
    }
    for (int i = 0; i < 128; i++) {
        packet->file[i] = 0;
    }
    
    // DHCP magic cookie
    packet->options[0] = 99;  // DHCP magic
    packet->options[1] = 130;
    packet->options[2] = 83;
    packet->options[3] = 99;
    
    // Add DHCP message type option
    int option_offset = 4;
    packet->options[option_offset++] = 53; // DHCP Message Type option
    packet->options[option_offset++] = 1;  // Length
    packet->options[option_offset++] = message_type; // Message type
    
    // Add Parameter Request List option
    packet->options[option_offset++] = 55; // Parameter Request List
    packet->options[option_offset++] = 4;  // Length
    packet->options[option_offset++] = 1;  // Subnet Mask
    packet->options[option_offset++] = 3;  // Router
    packet->options[option_offset++] = 6;  // Domain Name Server
    packet->options[option_offset++] = 15; // Domain Name
    
    // Add Client Identifier option (MAC address)
    packet->options[option_offset++] = 61; // Client Identifier
    packet->options[option_offset++] = 7;  // Length (1 + 6 for MAC)
    packet->options[option_offset++] = 1;  // Hardware type (Ethernet)
    uint8_t mac[6];
    network_get_mac_address(mac);
    for (int i = 0; i < 6; i++) {
        packet->options[option_offset++] = mac[i];
    }
    
    // End option
    packet->options[option_offset++] = 255; // End option
    
    // Pad remaining options with zeros
    while (option_offset < 312) {
        packet->options[option_offset++] = 0;
    }
    
    terminal_writestring("DHCP: Packet created successfully\n");
}

// Add this debug function at the top
static void dhcp_debug(const char* message) {
    extern void terminal_writestring(const char* data);
    terminal_writestring("DHCP DEBUG: ");
    terminal_writestring(message);
    terminal_writestring("\n");
}

// Send DHCP packet via UDP broadcast
static bool dhcp_send_packet(const dhcp_packet_t* packet) {
    extern void terminal_writestring(const char* data);
    
    if (!network_is_ready()) {
        terminal_writestring("DHCP: Network not ready\n");
        return false;
    }
    
    if (!rtl8139_is_initialized()) {
        terminal_writestring("DHCP: RTL8139 not initialized\n");
        return false;
    }
    
    terminal_writestring("DHCP: Building packet...\n");
    
    // Use static buffer - no dynamic allocation
    static uint8_t frame[1600];
    
    // Calculate sizes
    uint16_t dhcp_len = sizeof(dhcp_packet_t);
    uint16_t udp_len = sizeof(udp_header_t) + dhcp_len;
    uint16_t ip_len = sizeof(ip_header_t) + udp_len;
    uint16_t total_len = 14 + ip_len;
    
    if (total_len > sizeof(frame)) {
        terminal_writestring("DHCP: Packet too large\n");
        return false;
    }
    
    // Clear the frame
    for (uint16_t i = 0; i < total_len; i++) {
        frame[i] = 0;
    }
    
    // Build Ethernet header
    ethernet_frame_t* eth = (ethernet_frame_t*)frame;
    eth->dest_mac[0] = 0xFF; eth->dest_mac[1] = 0xFF; eth->dest_mac[2] = 0xFF;
    eth->dest_mac[3] = 0xFF; eth->dest_mac[4] = 0xFF; eth->dest_mac[5] = 0xFF;
    
    network_get_mac_address(eth->src_mac);
    eth->ethertype = htons(ETHERTYPE_IP);
    
    // Build IP header
    ip_header_t* ip = (ip_header_t*)(frame + 14);
    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = htons(ip_len);
    ip->identification = 0;
    ip->flags_fragment = 0;
    ip->ttl = 64;
    ip->protocol = IP_PROTOCOL_UDP;
    ip->checksum = 0;
    ip->src_ip = 0;
    ip->dest_ip = 0xFFFFFFFF;
    ip->checksum = network_checksum(ip, sizeof(ip_header_t));
    
    // Build UDP header
    udp_header_t* udp = (udp_header_t*)(frame + 14 + sizeof(ip_header_t));
    udp->src_port = htons(68);
    udp->dest_port = htons(67);
    udp->length = htons(udp_len);
    udp->checksum = 0;
    
    // Copy DHCP packet manually
    dhcp_packet_t* dhcp_pkt = (dhcp_packet_t*)(frame + 14 + sizeof(ip_header_t) + sizeof(udp_header_t));
    uint8_t* src = (uint8_t*)packet;
    uint8_t* dst = (uint8_t*)dhcp_pkt;
    for (size_t i = 0; i < sizeof(dhcp_packet_t); i++) {
        dst[i] = src[i];
    }
    
    terminal_writestring("DHCP: Sending packet...\n");
    
    bool result = rtl8139_send_packet(frame, total_len);
    
    if (result) {
        terminal_writestring("DHCP: Packet sent successfully\n");
    } else {
        terminal_writestring("DHCP: Send failed\n");
    }
    
    return result;
}

// Parse DHCP options
static void dhcp_parse_options(const uint8_t* options, uint16_t length, 
                              uint8_t* message_type, uint32_t* server_ip,
                              uint32_t* subnet_mask, uint32_t* router,
                              uint32_t* dns_server, uint32_t* lease_time) {
    const uint8_t* ptr = options + 4; // Skip magic cookie
    const uint8_t* end = options + length;
    
    while (ptr < end && *ptr != DHCP_OPTION_END) {
        uint8_t option = *ptr++;
        if (option == 0) continue; // Padding
        
        if (ptr >= end) break;
        uint8_t len = *ptr++;
        if (ptr + len > end) break;
        
        switch (option) {
            case DHCP_OPTION_MESSAGE_TYPE:
                if (len >= 1) *message_type = ptr[0];
                break;
                
            case DHCP_OPTION_SERVER_ID:
                if (len >= 4) {
                    *server_ip = (ptr[0] << 24) | (ptr[1] << 16) | 
                                (ptr[2] << 8) | ptr[3];
                }
                break;
                
            case DHCP_OPTION_SUBNET_MASK:
                if (len >= 4) {
                    *subnet_mask = (ptr[0] << 24) | (ptr[1] << 16) | 
                                  (ptr[2] << 8) | ptr[3];
                }
                break;
                
            case DHCP_OPTION_ROUTER:
                if (len >= 4) {
                    *router = (ptr[0] << 24) | (ptr[1] << 16) | 
                             (ptr[2] << 8) | ptr[3];
                }
                break;
                
            case DHCP_OPTION_DNS_SERVER:
                if (len >= 4) {
                    *dns_server = (ptr[0] << 24) | (ptr[1] << 16) | 
                                 (ptr[2] << 8) | ptr[3];
                }
                break;
                
            case DHCP_OPTION_LEASE_TIME:
                if (len >= 4) {
                    *lease_time = (ptr[0] << 24) | (ptr[1] << 16) | 
                                 (ptr[2] << 8) | ptr[3];
                }
                break;
        }
        
        ptr += len;
    }
}

// Process DHCP packet
void dhcp_process_packet(const uint8_t* packet, uint16_t length) {
    if (!dhcp_active || length < sizeof(dhcp_packet_t)) {
        return;
    }
    
    const dhcp_packet_t* dhcp = (const dhcp_packet_t*)packet;
    
    // Check if this is a response to our request
    if (dhcp->op != 2 || ntohl(dhcp->xid) != dhcp_xid) {
        return;
    }
    
        // Parse options
    uint8_t message_type = 0;
    uint32_t server_ip = 0;
    uint32_t subnet_mask = 0;
    uint32_t router = 0;
    uint32_t dns_server = 0;
    uint32_t lease_time = 0;
    
    dhcp_parse_options(dhcp->options, sizeof(dhcp->options), 
                      &message_type, &server_ip, &subnet_mask, 
                      &router, &dns_server, &lease_time);
    
    switch (dhcp_state) {
        case DHCP_STATE_SELECTING:
            if (message_type == DHCP_OFFER) {
                terminal_writestring("DHCP: Received OFFER from server\n");
                
                dhcp_offered_ip = ntohl(dhcp->yiaddr);
                dhcp_server_ip = server_ip;
                dhcp_lease_time = lease_time;
                dhcp_subnet_mask = subnet_mask;
                dhcp_router = router;
                dhcp_dns_server = dns_server;
                
                // Send REQUEST
                dhcp_state = DHCP_STATE_REQUESTING;
                dhcp_packet_t request_packet;
                dhcp_create_packet(&request_packet, DHCP_REQUEST);
                dhcp_send_packet(&request_packet);
                
                terminal_writestring("DHCP: Sent REQUEST for offered IP\n");
                dhcp_timer = 0;
            }
            break;
            
        case DHCP_STATE_REQUESTING:
            if (message_type == DHCP_ACK) {
                terminal_writestring("DHCP: Received ACK - Lease acquired!\n");
                
                // Apply network configuration
                network_set_ip(dhcp_offered_ip);
                if (dhcp_subnet_mask) network_set_netmask(dhcp_subnet_mask);
                if (dhcp_router) network_set_gateway(dhcp_router);
                
                dhcp_state = DHCP_STATE_BOUND;
                dhcp_timer = 0;
                
                // Show configuration
                char ip_str[16];
                ip_int_to_str(dhcp_offered_ip, ip_str);
                terminal_writestring("DHCP: IP Address: ");
                terminal_writestring(ip_str);
                terminal_writestring("\n");
                
                if (dhcp_subnet_mask) {
                    ip_int_to_str(dhcp_subnet_mask, ip_str);
                    terminal_writestring("DHCP: Subnet Mask: ");
                    terminal_writestring(ip_str);
                    terminal_writestring("\n");
                }
                
                if (dhcp_router) {
                    ip_int_to_str(dhcp_router, ip_str);
                    terminal_writestring("DHCP: Gateway: ");
                    terminal_writestring(ip_str);
                    terminal_writestring("\n");
                }
                
                if (dhcp_dns_server) {
                    ip_int_to_str(dhcp_dns_server, ip_str);
                    terminal_writestring("DHCP: DNS Server: ");
                    terminal_writestring(ip_str);
                    terminal_writestring("\n");
                }
                
                if (dhcp_lease_time) {
                    terminal_writestring("DHCP: Lease time: ");
                    // Simple number to string conversion
                    char time_str[16];
                    uint32_t time = dhcp_lease_time;
                    int pos = 0;
                    if (time == 0) {
                        time_str[pos++] = '0';
                    } else {
                        char temp[16];
                        int temp_pos = 0;
                        while (time > 0) {
                            temp[temp_pos++] = '0' + (time % 10);
                            time /= 10;
                        }
                        for (int i = temp_pos - 1; i >= 0; i--) {
                            time_str[pos++] = temp[i];
                        }
                    }
                    time_str[pos] = '\0';
                    terminal_writestring(time_str);
                    terminal_writestring(" seconds\n");
                }
                
            } else if (message_type == DHCP_NAK) {
                terminal_writestring("DHCP: Received NAK - Request denied\n");
                dhcp_state = DHCP_STATE_INIT;
                dhcp_timer = 0;
            }
            break;
            
        case DHCP_STATE_BOUND:
        case DHCP_STATE_RENEWING:
        case DHCP_STATE_REBINDING:
            if (message_type == DHCP_ACK) {
                terminal_writestring("DHCP: Lease renewed successfully\n");
                dhcp_state = DHCP_STATE_BOUND;
                dhcp_timer = 0;
                if (lease_time) dhcp_lease_time = lease_time;
            } else if (message_type == DHCP_NAK) {
                terminal_writestring("DHCP: Renewal denied - Restarting discovery\n");
                dhcp_state = DHCP_STATE_INIT;
                dhcp_timer = 0;
            }
            break;
            
        default:
            break;
    }
}

// Start DHCP client
bool dhcp_start(void) {
    if (dhcp_active) {
        return true; // Already running
    }
    
    terminal_writestring("DHCP: Starting client...\n");
    
    dhcp_active = true;
    dhcp_state = DHCP_STATE_INIT;
    dhcp_xid = dhcp_random();
    dhcp_timer = 0;
    dhcp_server_ip = 0;
    dhcp_offered_ip = 0;
    dhcp_lease_time = 0;
    dhcp_subnet_mask = 0;
    dhcp_router = 0;
    dhcp_dns_server = 0;
    
    // Clear current IP configuration
    network_set_ip(0);
    network_set_gateway(0);
    network_set_netmask(0);
    
    return true;
}

// Stop DHCP client
void dhcp_stop(void) {
    if (!dhcp_active) {
        return;
    }
    
    terminal_writestring("DHCP: Stopping client...\n");
    
    // Send RELEASE if we have a lease
    if (dhcp_state == DHCP_STATE_BOUND && dhcp_server_ip != 0) {
        dhcp_packet_t release_packet;
        dhcp_create_packet(&release_packet, DHCP_RELEASE);
        dhcp_send_packet(&release_packet);
        terminal_writestring("DHCP: Sent RELEASE to server\n");
    }
    
    dhcp_active = false;
    dhcp_state = DHCP_STATE_INIT;
}

// Check if DHCP is active
bool dhcp_is_active(void) {
    return dhcp_active;
}

// Get DHCP state
dhcp_state_t dhcp_get_state(void) {
    return dhcp_state;
}

// DHCP tick function - call this periodically (every second)
void dhcp_tick(void) {
    if (!dhcp_active) {
        return;
    }
    
    dhcp_timer++;
    
    switch (dhcp_state) {
        case DHCP_STATE_INIT:
            // Start discovery
            dhcp_debug("Starting DHCP discovery");
            dhcp_xid = dhcp_random();
            dhcp_state = DHCP_STATE_SELECTING;
            dhcp_timer = 0;
            
            // Send DISCOVER
            dhcp_packet_t discover_packet;
            dhcp_create_packet(&discover_packet, DHCP_DISCOVER);
            if (dhcp_send_packet(&discover_packet)) {
                terminal_writestring("DHCP: DISCOVER sent successfully\n");
            } else {
                terminal_writestring("DHCP: Failed to send DISCOVER\n");
            }
            break;
            
        case DHCP_STATE_SELECTING:
            // Wait for OFFER, timeout after 10 seconds
            if (dhcp_timer >= 10) {
                terminal_writestring("DHCP: DISCOVER timeout - retrying\n");
                dhcp_debug("No OFFER received, checking network status");
                
                // Check if network is still ready
                if (!network_is_ready()) {
                    dhcp_debug("Network interface is not ready");
                } else {
                    dhcp_debug("Network interface is ready, but no DHCP server responded");
                }
                
                dhcp_state = DHCP_STATE_INIT;
                dhcp_timer = 0;
            } else if (dhcp_timer % 2 == 0) {
                // Show progress every 2 seconds
                char timer_str[16];
                uint32_t remaining = 10 - dhcp_timer;
                // Simple number to string
                int pos = 0;
                if (remaining == 0) {
                    timer_str[pos++] = '0';
                } else {
                    char temp[16];
                    int temp_pos = 0;
                    uint32_t num = remaining;
                    while (num > 0) {
                        temp[temp_pos++] = '0' + (num % 10);
                        num /= 10;
                    }
                    for (int i = temp_pos - 1; i >= 0; i--) {
                        timer_str[pos++] = temp[i];
                    }
                }
                timer_str[pos] = '\0';
                
                terminal_writestring("DHCP: Waiting for OFFER... (");
                terminal_writestring(timer_str);
                terminal_writestring("s remaining)\n");
            }
            break;
            
        case DHCP_STATE_REQUESTING:
            // Wait for ACK, timeout after 10 seconds
            if (dhcp_timer >= 10) {
                terminal_writestring("DHCP: REQUEST timeout - retrying\n");
                dhcp_state = DHCP_STATE_INIT;
                dhcp_timer = 0;
            }
            break;
            
        case DHCP_STATE_BOUND:
            // Check if we need to renew (at 50% of lease time)
            if (dhcp_lease_time > 0 && dhcp_timer >= dhcp_lease_time / 2) {
                terminal_writestring("DHCP: Starting lease renewal...\n");
                dhcp_state = DHCP_STATE_RENEWING;
                dhcp_timer = 0;
                
                // Send REQUEST for renewal
                dhcp_packet_t request_packet;
                dhcp_create_packet(&request_packet, DHCP_REQUEST);
                dhcp_send_packet(&request_packet);
            }
            break;
            
        case DHCP_STATE_RENEWING:
            // Try to renew every 5 seconds, give up after 25% more of lease time
            if (dhcp_timer % 5 == 0) {
                dhcp_packet_t request_packet;
                dhcp_create_packet(&request_packet, DHCP_REQUEST);
                dhcp_send_packet(&request_packet);
            }
            
            if (dhcp_lease_time > 0 && dhcp_timer >= dhcp_lease_time / 4) {
                terminal_writestring("DHCP: Renewal failed - entering rebinding\n");
                dhcp_state = DHCP_STATE_REBINDING;
                dhcp_timer = 0;
            }
            break;
            
        case DHCP_STATE_REBINDING:
            // Try to rebind every 5 seconds, give up after remaining lease time
            if (dhcp_timer % 5 == 0) {
                dhcp_packet_t request_packet;
                dhcp_create_packet(&request_packet, DHCP_REQUEST);
                dhcp_send_packet(&request_packet);
            }
            
            if (dhcp_lease_time > 0 && dhcp_timer >= dhcp_lease_time / 4) {
                terminal_writestring("DHCP: Rebinding failed - restarting discovery\n");
                dhcp_state = DHCP_STATE_INIT;
                dhcp_timer = 0;
            }
            break;
            
        default:
            break;
    }
}

// Get state name for debugging
const char* dhcp_get_state_name(void) {
    switch (dhcp_state) {
        case DHCP_STATE_INIT: return "INIT";
        case DHCP_STATE_SELECTING: return "SELECTING";
        case DHCP_STATE_REQUESTING: return "REQUESTING";
        case DHCP_STATE_BOUND: return "BOUND";
        case DHCP_STATE_RENEWING: return "RENEWING";
        case DHCP_STATE_REBINDING: return "REBINDING";
        case DHCP_STATE_INIT_REBOOT: return "INIT_REBOOT";
        default: return "UNKNOWN";
    }
}
