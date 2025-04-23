#ifndef __KEYBOARD_H_
#define __KEYBOARD_H_
#include <stdint.h>

extern void keyboard_init();
extern uint8_t keyboard_enabled();
extern char keyboard_get_key();
extern uint8_t keyboard_to_ascii(uint8_t key);
extern void keyboard_read_key();  // Add this line
extern uint8_t lastkey;  // Add this line to declare the external variable

#endif
