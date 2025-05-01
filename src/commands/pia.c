#include "../include/filesystem.h"
#include "../include/string.h"
#include "../include/keyboard.h"
#include "../include/vga.h"
#include "../include/io.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

extern void terminal_writestring(const char* data);
extern void terminal_setcolor(uint8_t color);
extern void terminal_putchar(char c);
extern void terminal_clear(void);
extern void update_cursor(int row, int col);
extern char keyboard_get_key(void);
extern void keyboard_poll(void);

#define EDITOR_MAX_LINES 100
#define EDITOR_MAX_LINE_LENGTH 80

typedef struct {
    char lines[EDITOR_MAX_LINES][EDITOR_MAX_LINE_LENGTH];
    int num_lines;
    int cursor_x;
    int cursor_y;
    int screen_offset;
    char filename[FS_MAX_NAME_LENGTH];
    bool modified;
    bool exit_requested;  // Add this field
} editor_state_t;

// Function prototypes
void editor_init(editor_state_t* state, const char* filename);
void editor_load_file(editor_state_t* state);
void editor_save_file(editor_state_t* state);
void editor_display(editor_state_t* state);
void editor_handle_key(editor_state_t* state, char key);
void editor_insert_char(editor_state_t* state, char c);
void editor_delete_char(editor_state_t* state);
void editor_new_line(editor_state_t* state);
char editor_get_key(void);

#define KEY_ARROW_UP 0x80
#define KEY_ARROW_DOWN 0x81
#define KEY_ARROW_LEFT 0x82
#define KEY_ARROW_RIGHT 0x83
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEY_CTRL_X 24  // ASCII for Ctrl+X (CAN)

// Modify the keyboard_process_scancode function in kernel.c to handle arrow keys
// or create a special version for the editor:

// Add this function to pia.c
char editor_get_key(void) {
    // Poll keyboard for input
    keyboard_poll();
    
    // Check for special keys first
    if ((inb(KEYBOARD_STATUS_PORT) & 1) != 0) {
        uint8_t scancode = inb(KEYBOARD_DATA_PORT);
        
        // Arrow keys are extended keys (preceded by 0xE0)
        if (scancode == 0xE0) {
            // Read the next scancode
            scancode = inb(KEYBOARD_DATA_PORT);
            
            switch(scancode) {
                case 0x48: return KEY_ARROW_UP;
                case 0x50: return KEY_ARROW_DOWN;
                case 0x4B: return KEY_ARROW_LEFT;
                case 0x4D: return KEY_ARROW_RIGHT;
            }
        }

        // Check for Ctrl+X (Ctrl is usually scan code 0x1D)
        static bool ctrl_pressed = false;

        // In your scancode processing
        if (scancode == 0x1D) {  // Left Ctrl pressed
            ctrl_pressed = true;
        } else if (scancode == 0x9D) {  // Left Ctrl released
            ctrl_pressed = false;
        } else if (scancode == (0x1D | 0x80)) {
            // Ctrl key released
            ctrl_pressed = false;
        } else // In your keyboard handling code
        if (scancode == 0x2D && ctrl_pressed) {  // 'X' scancode with Ctrl
            return KEY_CTRL_X;
        }        
    }
    
    // Get regular key from the keyboard buffer
    return keyboard_get_key();
}

// Now update the editor_handle_key function to handle these special keys
void editor_handle_key(editor_state_t* state, char key) {
    // Handle different keys
    if (key == '\n') {
        // Enter key
        editor_new_line(state);
    } else if (key == '\b') {
        // Backspace
        editor_delete_char(state);
    } else if (key == KEY_CTRL_X) {
        // Ctrl+X - Save and exit
        editor_save_file(state);
        state->exit_requested = true;  // Add this flag to editor_state_t
    } else if (key == KEY_ARROW_UP) {
        // Up arrow - move cursor up
        if (state->cursor_y > 0) {
            state->cursor_y--;
            // Adjust cursor_x if the new line is shorter
            int line_len = strlen(state->lines[state->cursor_y]);
            if (state->cursor_x > line_len) {
                state->cursor_x = line_len;
            }
            
            // Adjust screen offset if needed
            if (state->cursor_y < state->screen_offset) {
                state->screen_offset = state->cursor_y;
            }
        }
    } else if (key == KEY_ARROW_DOWN) {
        // Down arrow - move cursor down
        if (state->cursor_y < state->num_lines - 1) {
            state->cursor_y++;
            // Adjust cursor_x if the new line is shorter
            int line_len = strlen(state->lines[state->cursor_y]);
            if (state->cursor_x > line_len) {
                state->cursor_x = line_len;
            }
            
            // Adjust screen offset if needed
            if (state->cursor_y >= state->screen_offset + VGA_HEIGHT - 2) {
                state->screen_offset = state->cursor_y - (VGA_HEIGHT - 3);
            }
        }
    } else if (key == KEY_ARROW_LEFT) {
        // Left arrow - move cursor left
        if (state->cursor_x > 0) {
            state->cursor_x--;
        } else if (state->cursor_y > 0) {
            // Move to end of previous line
            state->cursor_y--;
            state->cursor_x = strlen(state->lines[state->cursor_y]);
            
            // Adjust screen offset if needed
            if (state->cursor_y < state->screen_offset) {
                state->screen_offset = state->cursor_y;
            }
        }
    } else if (key == KEY_ARROW_RIGHT) {
        // Right arrow - move cursor right
        int line_len = strlen(state->lines[state->cursor_y]);
        if (state->cursor_x < line_len) {
            state->cursor_x++;
        } else if (state->cursor_y < state->num_lines - 1) {
            // Move to beginning of next line
            state->cursor_y++;
            state->cursor_x = 0;
            
            // Adjust screen offset if needed
            if (state->cursor_y >= state->screen_offset + VGA_HEIGHT - 2) {
                state->screen_offset = state->cursor_y - (VGA_HEIGHT - 3);
            }
        }
    } else if (key >= 32 && key <= 126) {
        // Printable character
        editor_insert_char(state, key);
    }
}

void cmd_pia(const char* args) {
    // Skip leading spaces
    while (*args == ' ') args++;
    
    // Check if a filename was provided
    if (*args == '\0') {
        terminal_writestring("Usage: pia <filename>\n");
        return;
    }
    
    // Initialize editor state
    editor_state_t state;
    editor_init(&state, args);
    
    // Load file if it exists
    editor_load_file(&state);
    
    // Clear screen and display editor
    terminal_clear();
    editor_display(&state);
    
    // Main editor loop
    bool running = true;
    while (running) {
        // Poll keyboard for input
        keyboard_poll();
        
        // Check for special keys first (arrow keys)
        if ((inb(KEYBOARD_STATUS_PORT) & 1) != 0) {
            uint8_t scancode = inb(KEYBOARD_DATA_PORT);
            
            // Check for extended keys (arrow keys)
            if (scancode == 0xE0) {
                // Extended key - read the next byte
                while ((inb(KEYBOARD_STATUS_PORT) & 1) == 0) {
                    // Wait for the next byte
                }
                
                scancode = inb(KEYBOARD_DATA_PORT);
                
                // Handle arrow keys
                switch (scancode) {
                    case 0x48: // Up arrow
                        if (state.cursor_y > 0) {
                            state.cursor_y--;
                            // Adjust cursor_x if the new line is shorter
                            int line_len = strlen(state.lines[state.cursor_y]);
                            if (state.cursor_x > line_len) {
                                state.cursor_x = line_len;
                            }
                            
                            // Adjust screen offset if needed
                            if (state.cursor_y < state.screen_offset) {
                                state.screen_offset = state.cursor_y;
                            }
                            editor_display(&state);
                        }
                        continue;
                        
                    case 0x50: // Down arrow
                        if (state.cursor_y < state.num_lines - 1) {
                            state.cursor_y++;
                            // Adjust cursor_x if the new line is shorter
                            int line_len = strlen(state.lines[state.cursor_y]);
                            if (state.cursor_x > line_len) {
                                state.cursor_x = line_len;
                            }
                            
                            // Adjust screen offset if needed
                            if (state.cursor_y >= state.screen_offset + VGA_HEIGHT - 2) {
                                state.screen_offset = state.cursor_y - (VGA_HEIGHT - 3);
                            }
                            editor_display(&state);
                        }
                        continue;
                        
                    case 0x4B: // Left arrow
                        if (state.cursor_x > 0) {
                            state.cursor_x--;
                        } else if (state.cursor_y > 0) {
                            // Move to end of previous line
                            state.cursor_y--;
                            state.cursor_x = strlen(state.lines[state.cursor_y]);
                            
                            // Adjust screen offset if needed
                            if (state.cursor_y < state.screen_offset) {
                                state.screen_offset = state.cursor_y;
                            }
                        }
                        editor_display(&state);
                        continue;
                        
                    case 0x4D: // Right arrow
                        int line_len = strlen(state.lines[state.cursor_y]);
                        if (state.cursor_x < line_len) {
                            state.cursor_x++;
                        } else if (state.cursor_y < state.num_lines - 1) {
                            // Move to beginning of next line
                            state.cursor_y++;
                            state.cursor_x = 0;
                            
                            // Adjust screen offset if needed
                            if (state.cursor_y >= state.screen_offset + VGA_HEIGHT - 2) {
                                state.screen_offset = state.cursor_y - (VGA_HEIGHT - 3);
                            }
                        }
                        editor_display(&state);
                        continue;
                }
            }
        }
        
        // Process regular keys from the keyboard buffer
        char key = keyboard_get_key();
        
        if (key != 0) {
            // Check for special key combinations
            if (key == 24) { // Ctrl+X (ASCII 24 = CAN)
                // Save and exit
                editor_save_file(&state);
                running = false;
            } else {
                // Handle regular key
                editor_handle_key(&state, key);
                editor_display(&state);
            }
        }
    }
    
    // Clear screen when exiting
    terminal_clear();
    terminal_writestring("File saved: ");
    terminal_writestring(state.filename);
    terminal_writestring("\n");
}

void editor_init(editor_state_t* state, const char* filename) {
    // Initialize state
    state->num_lines = 1;
    state->cursor_x = 0;
    state->cursor_y = 0;
    state->screen_offset = 0;
    state->modified = false;
    state->exit_requested = false;
    
    // Clear all lines
    for (int i = 0; i < EDITOR_MAX_LINES; i++) {
        state->lines[i][0] = '\0';
    }
    
    // Copy filename
    strncpy(state->filename, filename, FS_MAX_NAME_LENGTH - 1);
    state->filename[FS_MAX_NAME_LENGTH - 1] = '\0';
}

void editor_load_file(editor_state_t* state) {
    // Try to find the file
    fs_node_t* file = fs_find_node_by_path(state->filename);
    
    if (file && file->type == FS_TYPE_FILE) {
        // File exists, load its content
        const char* content = file->content;
        
        // Parse content into lines
        state->num_lines = 0;
        int line_pos = 0;
        
        for (int i = 0; content[i] != '\0' && state->num_lines < EDITOR_MAX_LINES; i++) {
            if (content[i] == '\n') {
                // End of line
                state->lines[state->num_lines][line_pos] = '\0';
                state->num_lines++;
                line_pos = 0;
            } else if (line_pos < EDITOR_MAX_LINE_LENGTH - 1) {
                // Add character to current line
                state->lines[state->num_lines][line_pos++] = content[i];
            }
        }
        
        // Add the last line if it doesn't end with a newline
        if (line_pos > 0 || state->num_lines == 0) {
            state->lines[state->num_lines][line_pos] = '\0';
            state->num_lines++;
        }
    } else {
        // New file, start with one empty line
        state->num_lines = 1;
        state->lines[0][0] = '\0';
    }
}

void editor_save_file(editor_state_t* state) {
    if (!state->modified) {
        return; // No changes to save
    }
    
    // Combine all lines into a single string
    char content[FS_MAX_CONTENT_LENGTH] = {0};
    int pos = 0;
    
    for (int i = 0; i < state->num_lines && pos < FS_MAX_CONTENT_LENGTH - 2; i++) {
        // Copy line content
        int line_len = strlen(state->lines[i]);
        for (int j = 0; j < line_len && pos < FS_MAX_CONTENT_LENGTH - 2; j++) {
            content[pos++] = state->lines[i][j];
        }
        
        // Add newline (except for the last line if it's empty)
        if (i < state->num_lines - 1 || state->lines[i][0] != '\0') {
            content[pos++] = '\n';
        }
    }
    
    content[pos] = '\0';
    
    // Write to file - use the filesystem API correctly
    fs_node_t* file = fs_find_node_by_path(state->filename);
    if (file && file->type == FS_TYPE_FILE) {
        // Update existing file
        fs_write_file(file, content);
    } else {
        // Create new file
        fs_node_t* new_file = fs_create_file(fs_get_cwd(), state->filename);
        if (new_file) {
            fs_write_file(new_file, content);
        }
    }
    
    state->modified = false;
}

void editor_display(editor_state_t* state) {
    terminal_clear();
    
    // Display status line
    terminal_setcolor(VGA_COLOR_BLACK | (VGA_COLOR_LIGHT_GREY << 4)); // Inverted colors
    terminal_writestring(" PIA Editor - ");
    terminal_writestring(state->filename);
    if (state->modified) {
        terminal_writestring(" [modified]");
    }
    
    // Pad the rest of the status line
    for (int i = 0; i < VGA_WIDTH - strlen(" PIA Editor - ") - strlen(state->filename) - 
                      (state->modified ? strlen(" [modified]") : 0); i++) {
        terminal_putchar(' ');
    }
    
    // Reset colors for main editor area
    terminal_setcolor(VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4));
    
    // Display lines
    int display_lines = VGA_HEIGHT - 2; // Reserve 2 lines for status
    for (int i = 0; i < display_lines; i++) {
        int line_idx = i + state->screen_offset;
        
        if (line_idx < state->num_lines) {
            terminal_writestring(state->lines[line_idx]);
        }
        
        // Move to next line
        if (i < display_lines - 1) {
            terminal_putchar('\n');
        }
    }
    
    // Display help line at the bottom
    terminal_setcolor(VGA_COLOR_BLACK | (VGA_COLOR_LIGHT_GREY << 4)); // Inverted colors
    terminal_writestring(" Ctrl+X: Save and Exit");
    
    // Pad the rest of the help line
    for (int i = 0; i < VGA_WIDTH - strlen(" Ctrl+X: Save and Exit"); i++) {
        terminal_putchar(' ');
    }
    
    // Reset colors
    terminal_setcolor(VGA_COLOR_LIGHT_GREY | (VGA_COLOR_BLACK << 4));
    
    // Update cursor position
    update_cursor(state->cursor_y - state->screen_offset + 1, state->cursor_x); // +1 for status line
}

void editor_insert_char(editor_state_t* state, char c) {
    // Get current line
    char* line = state->lines[state->cursor_y];
    int len = strlen(line);
    
    // Check if there's room for a new character
    if (len < EDITOR_MAX_LINE_LENGTH - 1) {
        // Shift characters to make room
        for (int i = len; i >= state->cursor_x; i--) {
            line[i + 1] = line[i];
        }
        
        // Insert the new character
        line[state->cursor_x] = c;
        state->cursor_x++;
        state->modified = true;
    }
}

void editor_delete_char(editor_state_t* state) {
    if (state->cursor_x > 0) {
        // Delete character at cursor - 1
        char* line = state->lines[state->cursor_y];
        int len = strlen(line);
        
        // Shift characters to close the gap
        for (int i = state->cursor_x - 1; i < len; i++) {
            line[i] = line[i + 1];
        }
        
        state->cursor_x--;
        state->modified = true;
    } else if (state->cursor_y > 0) {
        // At beginning of line, merge with previous line
        char* curr_line = state->lines[state->cursor_y];
        char* prev_line = state->lines[state->cursor_y - 1];
        int prev_len = strlen(prev_line);
        
        // Check if there's room in the previous line
        if (prev_len + strlen(curr_line) < EDITOR_MAX_LINE_LENGTH) {
            // Append current line to previous line
            strcat(prev_line, curr_line);
            
            // Shift all lines up
            for (int i = state->cursor_y; i < state->num_lines - 1; i++) {
                strcpy(state->lines[i], state->lines[i + 1]);
            }
            
            state->num_lines--;
            state->cursor_y--;
            state->cursor_x = prev_len;
            state->modified = true;
        }
    }
}

void editor_new_line(editor_state_t* state) {
    // Check if we can add a new line
    if (state->num_lines < EDITOR_MAX_LINES) {
        // Get current line
        char* line = state->lines[state->cursor_y];
        int len = strlen(line);
        
        // Make room for new line
        for (int i = state->num_lines; i > state->cursor_y + 1; i--) {
            strcpy(state->lines[i], state->lines[i - 1]);
        }
        
        // Split the current line
        if (state->cursor_x < len) {
            // Copy the part after cursor to the new line
            strcpy(state->lines[state->cursor_y + 1], line + state->cursor_x);
            
            // Truncate the current line
            line[state->cursor_x] = '\0';
        } else {
            // Just add an empty line
            state->lines[state->cursor_y + 1][0] = '\0';
        }
        
        state->num_lines++;
        state->cursor_y++;
        state->cursor_x = 0;
        state->modified = true;
        
        // Adjust screen offset if cursor would go off screen
        if (state->cursor_y - state->screen_offset >= VGA_HEIGHT - 2) {
            state->screen_offset++;
        }
    }
}
