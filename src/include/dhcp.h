#ifndef DHCP_H
#define DHCP_H

#include <stdint.h>
#include <stdbool.h>

// DHCP message types
#define DHCP_DISCOVER   1
#define DHCP_OFFER      2
#define DHCP_REQUEST    3
#define DHCP_DECLINE    4
#define DHCP_ACK        5
#define DHCP_NAK        6
#define DHCP_RELEASE    7
#define DHCP_INFORM     8

// DHCP option codes
#define DHCP_OPTION_SUBNET_MASK     1
#define DHCP_OPTION_ROUTER          3
#define DHCP_OPTION_DNS_SERVER      6
#define DHCP_OPTION_LEASE_TIME      51
#define DHCP_OPTION_MESSAGE_TYPE    53
#define DHCP_OPTION_SERVER_ID       54
#define DHCP_OPTION_END             255

// DHCP states
typedef enum {
    DHCP_STATE_INIT,
    DHCP_STATE_SELECTING,
    DHCP_STATE_REQUESTING,
    DHCP_STATE_BOUND,
    DHCP_STATE_RENEWING,
    DHCP_STATE_REBINDING,
    DHCP_STATE_INIT_REBOOT
} dhcp_state_t;

// DHCP packet structure
typedef struct {
    uint8_t op;
    uint8_t htype;
    uint8_t hlen;
    uint8_t hops;
    uint32_t xid;
    uint16_t secs;
    uint16_t flags;
    uint32_t ciaddr;
    uint32_t yiaddr;
    uint32_t siaddr;
    uint32_t giaddr;
    uint8_t chaddr[16];
    uint8_t sname[64];
    uint8_t file[128];
    uint8_t options[312];
} __attribute__((packed)) dhcp_packet_t;

// Function declarations
bool dhcp_start(void);
void dhcp_stop(void);
bool dhcp_is_active(void);
dhcp_state_t dhcp_get_state(void);
void dhcp_tick(void);
const char* dhcp_get_state_name(void);
void dhcp_create_packet(dhcp_packet_t* packet, uint8_t message_type);
void dhcp_process_packet(const uint8_t* packet, uint16_t length);

#endif /* DHCP_H */
