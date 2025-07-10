#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../include/string.h"
#include "../include/commands.h"
#include "../include/vga.h"
#include "../include/keyboard.h"
#include "../include/io.h"
#include "../include/filesystem.h"

// External functions from kernel
extern void terminal_writestring(const char* data);
extern void terminal_putchar(char c);
extern void terminal_setcolor(uint8_t color);
extern uint8_t terminal_getcolor(void);
extern void terminal_putentryat(char c, uint8_t color, size_t x, size_t y);
extern void update_cursor(int row, int col);
extern size_t terminal_row;
extern size_t terminal_column;

// Key definitions for special keys
#define KEY_ESCAPE 27
#define KEY_F1 0x3B
#define KEY_ARROW_UP 0x48
#define KEY_ARROW_DOWN 0x50
#define KEY_ARROW_LEFT 0x4B
#define KEY_ARROW_RIGHT 0x4D
#define KEY_CTRL_S 19
#define KEY_CTRL_O 15
#define KEY_CTRL_Q 17

// Editor constants
#define EDITOR_MAX_LINES 1000
#define EDITOR_MAX_COLS 256

// Editor state
typedef struct {
    char buffer[EDITOR_MAX_LINES][EDITOR_MAX_COLS];
    int cursor_x;
    int cursor_y;
    int scroll_x;
    int scroll_y;
    int lines;
    bool modified;
    char filename[256];
    bool has_filename;
    bool help_menu_open;
} editor_state_t;

static editor_state_t editor_state;

// Function declarations
void editor_init(void);
void editor_display(void);
void editor_handle_key(char key);
char editor_get_key(void);
void editor_insert_char(char c);
void editor_delete_char(void);
void editor_move_cursor(int dx, int dy);
void editor_scroll_if_needed(void);
void editor_load_file(const char* filename);
void editor_save_file(void);
void editor_display_help_menu(void);
void editor_handle_help_menu(void);

void editor_init(void) {
    // Clear the buffer
    for (int i = 0; i < EDITOR_MAX_LINES; i++) {
        for (int j = 0; j < EDITOR_MAX_COLS; j++) {
            editor_state.buffer[i][j] = '\0';
        }
    }
    
    editor_state.cursor_x = 0;
    editor_state.cursor_y = 0;
    editor_state.scroll_x = 0;
    editor_state.scroll_y = 0;
    editor_state.lines = 1;
    editor_state.modified = false;
    editor_state.has_filename = false;
    editor_state.help_menu_open = false;
    editor_state.filename[0] = '\0';
}

void editor_load_file(const char* filename) {
    if (!filename || strlen(filename) == 0) {
        return;
    }
    
    // Check if filesystem is mounted
    if (!fs_is_mounted()) {
        terminal_writestring("Error: FAT32 filesystem not mounted\n");
        return;
    }
    
    strcpy(editor_state.filename, filename);
    editor_state.has_filename = true;
    
    // Check if file exists
    if (!fs_exists(filename)) {
        // File doesn't exist, start with empty buffer
        editor_init();
        strcpy(editor_state.filename, filename);
        editor_state.has_filename = true;
        return;
    }
    
    // Open the file
    fs_file_handle_t* file = fs_open(filename, "r");
    if (!file) {
        terminal_writestring("Error: Failed to open file\n");
        return;
    }
    
    if (file->is_directory) {
        terminal_writestring("Error: Cannot edit directory\n");
        fs_close(file);
        return;
    }
    
    // Load file content into buffer
    editor_init();
    strcpy(editor_state.filename, filename);
    editor_state.has_filename = true;
    
    char buffer[512];
    size_t bytes_read;
    int line = 0, col = 0;
    
    while ((bytes_read = fs_read(file, buffer, sizeof(buffer))) > 0 && line < EDITOR_MAX_LINES) {
        for (size_t i = 0; i < bytes_read && line < EDITOR_MAX_LINES; i++) {
            if (buffer[i] == '\n') {
                line++;
                col = 0;
            } else if (col < EDITOR_MAX_COLS - 1) {
                editor_state.buffer[line][col] = buffer[i];
                col++;
            }
        }
    }
    
    editor_state.lines = line + 1;
    fs_close(file);
}

void editor_save_file(void) {
    if (!editor_state.has_filename) {
        terminal_writestring("No filename specified\n");
        return;
    }
    
    if (!fs_is_mounted()) {
        terminal_writestring("Error: FAT32 filesystem not mounted\n");
        return;
    }
    
    // Note: File writing is not fully implemented in FAT32 yet
    terminal_writestring("Error: File writing not yet implemented in FAT32\n");
    terminal_writestring("File would be saved as: ");
    terminal_writestring(editor_state.filename);
    terminal_writestring("\n");
    
    // TODO: Implement this when fs_write is fully available
}

char editor_get_key(void) {
    while (1) {
        keyboard_poll();
        char key = keyboard_get_key();
        if (key != 0) {
            return key;
        }
    }
}

void editor_insert_char(char c) {
    if (editor_state.cursor_y >= EDITOR_MAX_LINES || editor_state.cursor_x >= EDITOR_MAX_COLS - 1) {
        return;
    }
    
    // Shift characters to the right
    for (int i = EDITOR_MAX_COLS - 2; i > editor_state.cursor_x; i--) {
        editor_state.buffer[editor_state.cursor_y][i] = editor_state.buffer[editor_state.cursor_y][i - 1];
    }
    
    editor_state.buffer[editor_state.cursor_y][editor_state.cursor_x] = c;
    editor_state.cursor_x++;
    editor_state.modified = true;
}

void editor_delete_char(void) {
    if (editor_state.cursor_x > 0) {
        editor_state.cursor_x--;
        
        // Shift characters to the left
        for (int i = editor_state.cursor_x; i < EDITOR_MAX_COLS - 1; i++) {
            editor_state.buffer[editor_state.cursor_y][i] = editor_state.buffer[editor_state.cursor_y][i + 1];
        }
        editor_state.buffer[editor_state.cursor_y][EDITOR_MAX_COLS - 1] = '\0';
        editor_state.modified = true;
    } else if (editor_state.cursor_y > 0) {
        // Move to end of previous line
        editor_state.cursor_y--;
        editor_state.cursor_x = strlen(editor_state.buffer[editor_state.cursor_y]);
        editor_state.modified = true;
    }
}

void editor_move_cursor(int dx, int dy) {
    editor_state.cursor_x += dx;
    editor_state.cursor_y += dy;
    
    // Bounds checking
    if (editor_state.cursor_x < 0) {
        editor_state.cursor_x = 0;
    }
    if (editor_state.cursor_y < 0) {
        editor_state.cursor_y = 0;
    }
    if (editor_state.cursor_y >= editor_state.lines) {
        editor_state.cursor_y = editor_state.lines - 1;
    }
    
    // Adjust cursor_x based on line length
    int line_length = strlen(editor_state.buffer[editor_state.cursor_y]);
    if (editor_state.cursor_x > line_length) {
        editor_state.cursor_x = line_length;
    }
    
    editor_scroll_if_needed();
}

void editor_scroll_if_needed(void) {
    // Horizontal scrolling
    if (editor_state.cursor_x < editor_state.scroll_x) {
        editor_state.scroll_x = editor_state.cursor_x;
    }
    if (editor_state.cursor_x >= editor_state.scroll_x + VGA_WIDTH) {
        editor_state.scroll_x = editor_state.cursor_x - VGA_WIDTH + 1;
    }
    
    // Vertical scrolling
    if (editor_state.cursor_y < editor_state.scroll_y) {
        editor_state.scroll_y = editor_state.cursor_y;
    }
    if (editor_state.cursor_y >= editor_state.scroll_y + VGA_HEIGHT - 2) { // -2 for status line
        editor_state.scroll_y = editor_state.cursor_y - VGA_HEIGHT + 3;
    }
}

void editor_handle_key(char key) {
    if (key == KEY_CTRL_Q) {
        // Quit editor
        return;
    } else if (key == KEY_CTRL_S) {
        // Save file
        editor_save_file();
    } else if (key == KEY_CTRL_O) {
        // Open file (simplified)
        terminal_writestring("Open file feature not implemented\n");
    } else if (key == KEY_F1) {
        // Toggle help menu
        editor_state.help_menu_open = !editor_state.help_menu_open;
    } else if (key == '\n') {
        // Enter key - new line
        if (editor_state.cursor_y < EDITOR_MAX_LINES - 1) {
            editor_state.cursor_y++;
            editor_state.cursor_x = 0;
            if (editor_state.cursor_y >= editor_state.lines) {
                editor_state.lines = editor_state.cursor_y + 1;
            }
                        editor_state.modified = true;
        }
    } else if (key == '\b') {
        // Backspace
        editor_delete_char();
    }
    else if (key == KEY_ARROW_UP) {
        editor_move_cursor(0, -1);
    } else if (key == KEY_ARROW_DOWN) {
        editor_move_cursor(0, 1);
    } else if (key == KEY_ARROW_LEFT) {
        editor_move_cursor(-1, 0);
    } else if (key == KEY_ARROW_RIGHT) {
        editor_move_cursor(1, 0);
    } else if (key >= 32 && key <= 126) {
        // Printable character
        editor_insert_char(key);
    }
    
    editor_scroll_if_needed();
}

void editor_display(void) {
    // Clear screen
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_putentryat(' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK), x, y);
        }
    }
    
    // Display buffer content
    for (int line = 0; line < VGA_HEIGHT - 2 && line + editor_state.scroll_y < editor_state.lines; line++) {
        int buffer_line = line + editor_state.scroll_y;
        for (int col = 0; col < VGA_WIDTH && col + editor_state.scroll_x < EDITOR_MAX_COLS; col++) {
            int buffer_col = col + editor_state.scroll_x;
            char c = editor_state.buffer[buffer_line][buffer_col];
            if (c == '\0') break;
            terminal_putentryat(c, vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK), col, line);
        }
    }
    
    // Display status line
    terminal_putentryat(' ', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), 0, VGA_HEIGHT - 2);
    
    // Status line content
    const char* status = editor_state.has_filename ? editor_state.filename : "[No Name]";
    for (size_t i = 0; i < strlen(status) && i < VGA_WIDTH; i++) {
        terminal_putentryat(status[i], vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), i, VGA_HEIGHT - 2);
    }
    
    if (editor_state.modified) {
        terminal_putentryat('*', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), strlen(status), VGA_HEIGHT - 2);
    }
    
    // Fill rest of status line
    for (size_t i = strlen(status) + (editor_state.modified ? 1 : 0); i < VGA_WIDTH; i++) {
        terminal_putentryat(' ', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), i, VGA_HEIGHT - 2);
    }
    
    // Display help line
    const char* help_text = " F1: Help | Ctrl+S: Save | Ctrl+Q: Quit";
    for (size_t i = 0; i < strlen(help_text) && i < VGA_WIDTH; i++) {
        terminal_putentryat(help_text[i], vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE), i, VGA_HEIGHT - 1);
    }
    
    // Fill rest of help line
    for (size_t i = strlen(help_text); i < VGA_WIDTH; i++) {
        terminal_putentryat(' ', vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE), i, VGA_HEIGHT - 1);
    }
    
    // Position cursor
    int screen_x = editor_state.cursor_x - editor_state.scroll_x;
    int screen_y = editor_state.cursor_y - editor_state.scroll_y;
    
    if (screen_x >= 0 && screen_x < VGA_WIDTH && screen_y >= 0 && screen_y < VGA_HEIGHT - 2) {
        update_cursor(screen_y, screen_x);
    }
}

void editor_display_help_menu(void) {
    // Display help menu overlay
    int start_x = 10;
    int start_y = 5;
    int width = 60;
    int height = 15;
    
    // Draw background
    for (int y = start_y; y < start_y + height && y < VGA_HEIGHT; y++) {
        for (int x = start_x; x < start_x + width && x < VGA_WIDTH; x++) {
            terminal_putentryat(' ', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), x, y);
        }
    }
    
    // Draw border
    for (int x = start_x; x < start_x + width && x < VGA_WIDTH; x++) {
        terminal_putentryat('-', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), x, start_y);
        terminal_putentryat('-', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), x, start_y + height - 1);
    }
    
    for (int y = start_y; y < start_y + height && y < VGA_HEIGHT; y++) {
        terminal_putentryat('|', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), start_x, y);
        terminal_putentryat('|', vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), start_x + width - 1, y);
    }
    
    // Display help content
    const char* title = "PIA Text Editor - Help";
    for (size_t i = 0; i < strlen(title) && start_x + 2 + i < VGA_WIDTH; i++) {
        terminal_putentryat(title[i], vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), start_x + 2 + i, start_y + 2);
    }
    
    const char* option1 = "Ctrl+S: Save file";
    const char* option2 = "Ctrl+O: Open file";
    const char* option3 = "Ctrl+Q: Quit editor";
    const char* option4 = "F1: Toggle this help menu";
    const char* option5 = "Arrow keys: Navigate";
    
    for (size_t i = 0; i < strlen(option1) && start_x + 2 + i < VGA_WIDTH; i++) {
        terminal_putentryat(option1[i], vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), start_x + 2 + i, start_y + 4);
    }
    
    for (size_t i = 0; i < strlen(option2) && start_x + 2 + i < VGA_WIDTH; i++) {
        terminal_putentryat(option2[i], vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), start_x + 2 + i, start_y + 5);
    }
    
    for (size_t i = 0; i < strlen(option3) && start_x + 2 + i < VGA_WIDTH; i++) {
        terminal_putentryat(option3[i], vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), start_x + 2 + i, start_y + 6);
    }
    
    for (size_t i = 0; i < strlen(option4) && start_x + 2 + i < VGA_WIDTH; i++) {
        terminal_putentryat(option4[i], vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), start_x + 2 + i, start_y + 7);
    }
    
    for (size_t i = 0; i < strlen(option5) && start_x + 2 + i < VGA_WIDTH; i++) {
        terminal_putentryat(option5[i], vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY), start_x + 2 + i, start_y + 8);
    }
}

void editor_handle_help_menu(void) {
    while (editor_state.help_menu_open) {
        editor_display();
        editor_display_help_menu();
        
        keyboard_poll();
        char key = keyboard_get_key();
        
        if (key == KEY_F1 || key == KEY_ESCAPE) {
            editor_state.help_menu_open = false;
        }
    }
}

void cmd_pia(const char* args) {
    // Initialize editor
    editor_init();
    
    // If filename provided, load it
    if (args && strlen(args) > 0) {
        // Skip leading spaces
        while (*args == ' ') args++;
        
        if (strlen(args) > 0) {
            editor_load_file(args);
        }
    }
    
    // Main editor loop
    while (1) {
        if (editor_state.help_menu_open) {
            editor_handle_help_menu();
        } else {
            editor_display();
            
            keyboard_poll();
            char key = keyboard_get_key();
            
            if (key == KEY_CTRL_Q) {
                break; // Exit editor
            }
            
            if (key != 0) {
                editor_handle_key(key);
            }
        }
    }
    
    // Clear screen and return to terminal
    for (size_t y = 0; y < VGA_HEIGHT; y++) {
        for (size_t x = 0; x < VGA_WIDTH; x++) {
            terminal_putentryat(' ', vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK), x, y);
        }
    }
    
    terminal_row = 0;
    terminal_column = 0;
}
