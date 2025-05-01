#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/string.h"
#include "../include/keyboard.h"
#include "../include/commands.h"
#include "../include/vga.h"

/* Check if the compiler thinks you are targeting the wrong operating system. */
#if defined(__linux__)
#error "You are not using a cross-compiler, you will most certainly run into trouble"
#endif

/* This tutorial will only work for the 32-bit ix86 targets. */
#if !defined(__i386__)
#error "This tutorial needs to be compiled with a ix86-elf compiler"
#endif

size_t terminal_row;
size_t terminal_column;
uint8_t terminal_color;
uint16_t* terminal_buffer = (uint16_t*)VGA_MEMORY;

/* Port I/O functions */
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile ("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

size_t strlen(const char* str) 
{
    size_t len = 0;
    while (str[len])
        len++;
    return len;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        ++s1;
        ++s2;
        --n;
    }
    if (n == 0) {
        return 0;
    }
    return (*(unsigned char *)s1 - *(unsigned char *)s2);
}
 
#define MAX_COMMAND_LENGTH 256
char current_command[MAX_COMMAND_LENGTH];
size_t command_length = 0;

void terminal_initialize(void) 
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
}

void terminal_setcolor(uint8_t color) 
{
    terminal_color = color;
}

void terminal_putentryat(char c, uint8_t color, size_t x, size_t y) 
{
    const size_t index = y * VGA_WIDTH + x;
    terminal_buffer[index] = vga_entry(c, color);
}

void terminal_putchar(char c) {
    if (c == '\n') {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
        return;
    }
    
    if (c == '\b') {  // Backspace handling
        if (terminal_column > 0) {
            terminal_column--;
            terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
        }
        return;
    }

    terminal_putentryat(c, terminal_color, terminal_column, terminal_row);
    if (++terminal_column == VGA_WIDTH) {
        terminal_column = 0;
        if (++terminal_row == VGA_HEIGHT)
            terminal_row = 0;
    }
}

void terminal_write(const char* data, size_t size) 
{
    for (size_t i = 0; i < size; i++)
        terminal_putchar(data[i]);
}

void terminal_writestring(const char* data) 
{
    terminal_write(data, strlen(data));
}

/* Keyboard driver implementation */
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEYBOARD_BUFFER_SIZE 16

static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static uint8_t buffer_head = 0;
static uint8_t buffer_tail = 0;
static bool shift_pressed = false;

/* US keyboard layout scancode to ASCII mapping */
static const char keymap[128] = {
    0, 0, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=', '\b',
    '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',
    0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Shifted keymap for uppercase and symbols */
static const char keymap_shifted[128] = {
    0, 0, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t', 'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', '\n',
    0, 'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~',
    0, '|', 'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0, '*',
    0, ' ', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

/* Initialize the keyboard */
void keyboard_init(void) {
    buffer_head = 0;
    buffer_tail = 0;
    shift_pressed = false;
}

/* Process a keyboard scancode */
void keyboard_process_scancode(uint8_t scancode) {
    /* Check if this is a key release (bit 7 set) */
    if (scancode & 0x80) {
        scancode &= 0x7F; /* Clear the release bit */
        
        /* Check if it's a shift key */
        if (scancode == 0x2A || scancode == 0x36) { /* Left or right shift */
            shift_pressed = false;
        }
    } else {
        /* Key press */
        /* Check for special keys first */
        if (scancode == 0x2A || scancode == 0x36) { /* Left or right shift */
            shift_pressed = true;
        } else if (scancode < 128) {
            /* Regular key, convert to ASCII */
            char ascii;
            if (shift_pressed) {
                ascii = keymap_shifted[scancode];
            } else {
                ascii = keymap[scancode];
            }
            
            /* Only add to buffer if it's a printable character */
            if (ascii != 0) {
                /* Add to circular buffer */
                keyboard_buffer[buffer_head] = ascii;
                buffer_head = (buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
                
                /* If buffer is full, advance tail */
                if (buffer_head == buffer_tail) {
                    buffer_tail = (buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
                }
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

void update_cursor(int row, int col) {
    unsigned short position = (row * VGA_WIDTH) + col;

    // Cursor Low Port (0x3D4)
    outb(0x3D4, 0x0F);
    outb(0x3D5, (unsigned char)(position & 0xFF));
    // Cursor High Port (0x3D4)
    outb(0x3D4, 0x0E);
    outb(0x3D5, (unsigned char)((position >> 8) & 0xFF));
}

void process_command(const char* cmd) {
    // Skip leading spaces
    while (*cmd == ' ') cmd++;
    
    // Find the end of the command (first space or null terminator)
    const char* cmd_end = cmd;
    while (*cmd_end && *cmd_end != ' ') cmd_end++;
    
    size_t cmd_length = cmd_end - cmd;
    
    // Check for empty command
    if (cmd_length == 0) {
        print_prompt();
        return;
    }
    
    // Check for "clear" command
    if (strncmp(cmd, "clear", cmd_length) == 0 && cmd_length == 5) {
        terminal_clear();
        print_prompt();
        return;
    }
    
    // Unknown command
    terminal_writestring("Unknown command: ");
    terminal_write(cmd, cmd_length);
    terminal_writestring("\n");
    print_prompt();
}

// Add this function to print the prompt
void print_prompt(void) {
    terminal_setcolor(VGA_COLOR_LIGHT_GREEN);
    terminal_writestring("admin@syncwideos");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring(":");
    terminal_setcolor(VGA_COLOR_CYAN);
    terminal_writestring("~");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("$ ");
    update_cursor(terminal_row, terminal_column);
}

void kernel_main(void) {
    /* Initialize terminal interface */
    terminal_initialize();

    /* Existing terminal output code */
    terminal_setcolor(VGA_COLOR_LIGHT_CYAN);
    terminal_writestring("------------------------------------\n");
    terminal_writestring("|      Welcome to SyncWide OS      |\n");
    terminal_writestring("|       https://os.syncwi.de       |\n");
    terminal_writestring("|                                  |\n");
    terminal_writestring("|         Version: 0.1.0gb         |\n");
    terminal_writestring("------------------------------------\n");
    terminal_setcolor(VGA_COLOR_LIGHT_GREY);
    terminal_writestring("\n");
    
    // Print initial prompt
    print_prompt();
    
    // Initialize keyboard
    keyboard_init();
    
    // Initialize command buffer
    command_length = 0;
    current_command[0] = '\0';

    // Main input loop
    while (1) {
        // Poll keyboard for input
        keyboard_poll();
        
        // Process any available keys
        char key = keyboard_get_key();
        
        if (key != 0) {
            // Handle special keys or control characters
            if (key == '\n') {
                // Enter key pressed - execute command
                terminal_writestring("\n");
                current_command[command_length] = '\0';
                process_command(current_command);
                
                // Reset command buffer
                command_length = 0;
                current_command[0] = '\0';
            } 
            else if (key == '\b') {
                // Backspace key pressed
                if (command_length > 0 && terminal_column > 0) {
                    command_length--;
                    current_command[command_length] = '\0';
                    terminal_column--;
                    terminal_putentryat(' ', terminal_color, terminal_column, terminal_row);
                    update_cursor(terminal_row, terminal_column);
                }
            }
            else {
                // Regular character input
                if (command_length < MAX_COMMAND_LENGTH - 1) {
                    current_command[command_length++] = key;
                    current_command[command_length] = '\0';
                    terminal_putchar(key);
                    update_cursor(terminal_row, terminal_column);
                }
            }
        }
    }    
}
