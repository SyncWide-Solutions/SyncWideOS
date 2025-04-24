/* This is a simple command that clears the screen. */
/* This command does nothing currently because its just to demonstrate how to strucure the command folder. */

/*void terminal_clear(void) 
{
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
}*/