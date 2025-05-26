#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/keyboard.h"
#include "../include/io.h"

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static uint8_t buffer_head = 0;
static uint8_t buffer_tail = 0;
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool alt_pressed = false;
static bool extended_key = false;

/* German keyboard layout scancode to ASCII mapping */
static const char keymap[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', 'ß', '´', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'z', 'u', 'i', 'o', 'p', 'ü', '+', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', 'ö', 'ä', '^',
    0, '#', 'y', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '-', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* German shifted keymap for uppercase and symbols */
static const char keymap_shifted[128] = {
    0, 0, '!', '"', '§', '$', '%', '&', '/', '(', ')', '=', '?', '`', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Z', 'U', 'I', 'O', 'P', 'Ü', '*', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', 'Ö', 'Ä', '°',
    0, '\'', 'Y', 'X', 'C', 'V', 'B', 'N', 'M', ';', ':', '_', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Initialize the keyboard */
void keyboard_init(void) {
    buffer_head = 0;
    buffer_tail = 0;
    shift_pressed = false;
    ctrl_pressed = false;
    alt_pressed = false;
    extended_key = false;
}

/* Add a key to the buffer */
static void add_key_to_buffer(char key) {
    keyboard_buffer[buffer_head] = key;
    buffer_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    
    /* If buffer is full, advance tail */
    if (buffer_head == buffer_tail) {
        buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    }
}

/* Process a keyboard scancode */
void keyboard_process_scancode(uint8_t scancode) {
    /* Check for extended key sequence */
    if (scancode == 0xE0) {
        extended_key = true;
        return;
    }
    
    /* Check if this is a key release (bit 7 set) */
    if (scancode & 0x80) {
        scancode &= 0x7F; /* Clear the release bit */
        
        /* Handle key releases */
        if (extended_key) {
            extended_key = false;
            /* Extended key release - handle special keys */
            switch (scancode) {
                case 0x1D: /* Right Ctrl */
                    ctrl_pressed = false;
                    break;
                case 0x38: /* Right Alt */
                    alt_pressed = false;
                    break;
            }
        } else {
            /* Regular key release */
            switch (scancode) {
                case 0x2A: /* Left Shift */
                case 0x36: /* Right Shift */
                    shift_pressed = false;
                    break;
                case 0x1D: /* Left Ctrl */
                    ctrl_pressed = false;
                    break;
                case 0x38: /* Left Alt */
                    alt_pressed = false;
                    break;
            }
        }
    } else {
        /* Key press */
        if (extended_key) {
            extended_key = false;
            /* Handle extended keys */
            switch (scancode) {
                case 0x48: /* Arrow Up */
                    add_key_to_buffer(KEY_ARROW_UP);
                    break;
                case 0x50: /* Arrow Down */
                    add_key_to_buffer(KEY_ARROW_DOWN);
                    break;
                case 0x4B: /* Arrow Left */
                    add_key_to_buffer(KEY_ARROW_LEFT);
                    break;
                case 0x4D: /* Arrow Right */
                    add_key_to_buffer(KEY_ARROW_RIGHT);
                    break;
                case 0x47: /* Home */
                    add_key_to_buffer(KEY_HOME);
                    break;
                case 0x4F: /* End */
                    add_key_to_buffer(KEY_END);
                    break;
                case 0x49: /* Page Up */
                    add_key_to_buffer(KEY_PAGE_UP);
                    break;
                case 0x51: /* Page Down */
                    add_key_to_buffer(KEY_PAGE_DOWN);
                    break;
                case 0x52: /* Insert */
                    add_key_to_buffer(KEY_INSERT);
                    break;
                case 0x53: /* Delete */
                    add_key_to_buffer(KEY_DELETE);
                    break;
                case 0x1D: /* Right Ctrl */
                    ctrl_pressed = true;
                    break;
                case 0x38: /* Right Alt */
                    alt_pressed = true;
                    break;
            }
        } else {
            /* Regular key press */
            switch (scancode) {
                case 0x2A: /* Left Shift */
                case 0x36: /* Right Shift */
                    shift_pressed = true;
                    break;
                case 0x1D: /* Left Ctrl */
                    ctrl_pressed = true;
                    break;
                case 0x38: /* Left Alt */
                    alt_pressed = true;
                    break;
                case 0x3B: /* F1 */
                    add_key_to_buffer(KEY_F1);
                    break;
                case 0x3C: /* F2 */
                    add_key_to_buffer(KEY_F2);
                    break;
                case 0x3D: /* F3 */
                    add_key_to_buffer(KEY_F3);
                    break;
                case 0x3E: /* F4 */
                    add_key_to_buffer(KEY_F4);
                    break;
                case 0x3F: /* F5 */
                    add_key_to_buffer(KEY_F5);
                    break;
                case 0x40: /* F6 */
                    add_key_to_buffer(KEY_F6);
                    break;
                case 0x41: /* F7 */
                    add_key_to_buffer(KEY_F7);
                    break;
                case 0x42: /* F8 */
                    add_key_to_buffer(KEY_F8);
                    break;
                case 0x43: /* F9 */
                    add_key_to_buffer(KEY_F9);
                    break;
                case 0x44: /* F10 */
                    add_key_to_buffer(KEY_F10);
                    break;
                case 0x57: /* F11 */
                    add_key_to_buffer(KEY_F11);
                    break;
                case 0x58: /* F12 */
                    add_key_to_buffer(KEY_F12);
                    break;
                default:
                    if (scancode < 128) {
                        /* Regular key, convert to ASCII */
                        char ascii;
                        if (shift_pressed) {
                            ascii = keymap_shifted[scancode];
                        } else {
                            ascii = keymap[scancode];
                        }
                        
                        /* Only add to buffer if it's a printable character */
                        if (ascii != 0) {
                            add_key_to_buffer(ascii);
                        }
                    }
                    break;
            }
        }
    }
}

/* Get a key from the keyboard buffer */
char keyboard_get_key(void) {
    /* Check if buffer is empty */
    if (buffer_head == buffer_tail) {
        return 0; /* No key available */
    }
    
    /* Get key from buffer */
    char key = keyboard_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
    
    return key;
}

/* Poll the keyboard for input */
void keyboard_poll(void) {
    /* Check if a key is available */
    if ((inb(KEYBOARD_STATUS_PORT) & 1) != 0) {
        /* Read the scancode */
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        keyboard_process_scancode(scancode);
    }
}
