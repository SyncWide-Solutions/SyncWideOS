#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#define KEY_CTRL_X 24
#define KEY_F1 132
#define KEY_ARROW_UP 1
#define KEY_ARROW_DOWN 2
#define KEY_ARROW_LEFT 3
#define KEY_ARROW_RIGHT 4

// Initialize the keyboard driver
void keyboard_init(void);

// Get a key from the keyboard buffer
// Returns 0 if no key is available
char keyboard_get_key(void);

// Poll the keyboard for input
void keyboard_poll(void);

// Process a keyboard scancode
void keyboard_process_scancode(uint8_t scancode);

#endif /* KEYBOARD_H */
