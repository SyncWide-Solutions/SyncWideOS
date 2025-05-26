#ifndef NETWORK_H
#define NETWORK_H

#include <stdint.h>
#include <stdbool.h>

// Ethernet constants
#define ETHERTYPE_IP    0x0800
#define ETHERTYPE_ARP   0x0806

// IP protocol constants
#define IP_PROTOCOL_ICMP 1
#define IP_PROTOCOL_TCP  6
#define IP_PROTOCOL_UDP  17

// ARP constants
#define ARP_HARDWARE_ETHERNET 1
#define ARP_OPERATION_REQUEST 1
#define ARP_OPERATION_REPLY   2

// Network structures
typedef struct {
    uint8_t dest_mac[6];
    uint8_t src_mac[6];
    uint16_t ethertype;
} __attribute__((packed)) ethernet_frame_t;

typedef struct {
    uint8_t version_ihl;
    uint8_t tos;
    uint16_t total_length;
    uint16_t identification;
    uint16_t flags_fragment;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t src_ip;
    uint32_t dest_ip;
} __attribute__((packed)) ip_header_t;

typedef struct {
    uint16_t src_port;
    uint16_t dest_port;
    uint16_t length;
    uint16_t checksum;
} __attribute__((packed)) udp_header_t;

typedef struct {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
    uint16_t identifier;
    uint16_t sequence;
} __attribute__((packed)) icmp_header_t;

typedef struct {
    uint16_t hardware_type;
    uint16_t protocol_type;
    uint8_t hardware_len;
    uint8_t protocol_len;
    uint16_t operation;
    uint8_t sender_mac[6];
    uint32_t sender_ip;
    uint8_t target_mac[6];
    uint32_t target_ip;
} __attribute__((packed)) arp_packet_t;

// Include DHCP definitions
#include "dhcp.h"

// Network functions
void network_init(void);
bool network_is_ready(void);
void network_process_packets(void);

// Network configuration
void network_set_ip(uint32_t ip);
void network_set_gateway(uint32_t gateway);
void network_set_netmask(uint32_t mask);
uint32_t network_get_ip(void);
uint32_t network_get_gateway(void);
uint32_t network_get_netmask(void);
void network_get_mac_address(uint8_t* mac);

// Network utility functions
uint16_t htons(uint16_t hostshort);
uint16_t ntohs(uint16_t netshort);
uint32_t htonl(uint32_t hostlong);
uint32_t ntohl(uint32_t netlong);
uint16_t network_checksum(const void* data, size_t length);
uint32_t ip_str_to_int(const char* ip_str);
void ip_int_to_str(uint32_t ip, char* str);

// Packet sending functions
bool network_send_ethernet_frame(const uint8_t* dest_mac, uint16_t ethertype, const void* payload, uint16_t payload_len);
bool network_send_ip_packet(uint32_t dest_ip, uint8_t protocol, const void* payload, uint16_t payload_len);
bool network_send_udp_packet(uint32_t dest_ip, uint16_t dest_port, uint16_t src_port, const void* data, uint16_t data_len);
bool network_send_icmp_packet(uint32_t dest_ip, uint8_t type, uint8_t code, uint16_t id, uint16_t seq, const void* data, uint16_t data_len);

// ARP functions
bool arp_resolve(uint32_t ip, uint8_t* mac);
void arp_send_request(uint32_t target_ip);
void arp_process_packet(const arp_packet_t* arp);

// Network statistics
void network_get_stats(uint32_t* p_sent, uint32_t* p_received, uint32_t* b_sent, uint32_t* b_received);
void network_update_stats(bool sent, uint16_t bytes, bool error);

// DHCP function (implementation in network.c)
bool network_dhcp_request(void);

#endif /* NETWORK_H */
