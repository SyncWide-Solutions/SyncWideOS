#ifndef RTL8139_H
#define RTL8139_H

#include <stdint.h>
#include <stdbool.h>

// RTL8139 driver functions
bool rtl8139_init(void);
bool rtl8139_is_initialized(void);
void rtl8139_get_mac_address(uint8_t* mac);
bool rtl8139_send_packet(const uint8_t* data, uint16_t length);
int rtl8139_receive_packet(uint8_t* buffer, uint16_t max_length);

#endif /* RTL8139_H */
