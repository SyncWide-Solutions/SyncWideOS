#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>
#include <stdbool.h>

// Keyboard constants
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_BUFFER_SIZE 16

// Special key codes
#define KEY_ARROW_UP    0x80
#define KEY_ARROW_DOWN  0x81
#define KEY_ARROW_LEFT  0x82
#define KEY_ARROW_RIGHT 0x83
#define KEY_F1          0x84
#define KEY_F2          0x85
#define KEY_F3          0x86
#define KEY_F4          0x87
#define KEY_F5          0x88
#define KEY_F6          0x89
#define KEY_F7          0x8A
#define KEY_F8          0x8B
#define KEY_F9          0x8C
#define KEY_F10         0x8D
#define KEY_F11         0x8E
#define KEY_F12         0x8F
#define KEY_HOME        0x90
#define KEY_END         0x91
#define KEY_PAGE_UP     0x92
#define KEY_PAGE_DOWN   0x93
#define KEY_INSERT      0x94
#define KEY_DELETE      0x95

// Function declarations
void keyboard_init(void);
void keyboard_process_scancode(uint8_t scancode);
char keyboard_get_key(void);
void keyboard_poll(void);

#endif /* KEYBOARD_H */
