#include "../include/commands.h"
#include "../include/vga.h"

extern size_t terminal_row;
extern size_t terminal_column;
extern uint8_t terminal_color;
extern uint16_t* terminal_buffer;
extern uint16_t vga_entry(unsigned char uc, uint8_t color);
extern void update_cursor(int row, int col);

#define VGA_WIDTH   80
#define VGA_HEIGHT  25

void terminal_clear(void) {
    // Reset cursor position to top-left corner
    terminal_row = 0;
    terminal_column = 0;
    
    // Fill the entire screen with blank spaces using the current terminal color
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            const size_t index = y * VGA_WIDTH + x;
            terminal_buffer[index] = vga_entry(' ', terminal_color);
        }
    }
    
    // Update cursor to the top-left corner
    update_cursor(terminal_row, terminal_column);
}