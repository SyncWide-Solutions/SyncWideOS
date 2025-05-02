#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../include/string.h"
#include "../include/vga.h"
#include "../include/io.h"
#include "../include/filesystem.h"
#include "../include/keyboard.h"
#include "../include/vga.h"

// Editor constants
#define EDITOR_MAX_LINES 100
#define EDITOR_MAX_LINE_LENGTH 80
#define KEYBOARD_DATA_PORT 0x60
#define KEYBOARD_STATUS_PORT 0x64
#define KEY_CTRL_X 24  // ASCII for Ctrl+X (CAN)
#define KEY_F1 0x90  // Custom code for F1

// Editor state structure
typedef struct {
    char lines[EDITOR_MAX_LINES][EDITOR_MAX_LINE_LENGTH];
    int num_lines;
    int cursor_x;
    int cursor_y;
    int screen_offset;
    char filename[FS_MAX_NAME_LENGTH];
    bool modified;
    bool exit_requested;
    fs_node_t* file_node;
} editor_state_t;

// Function declarations
void editor_init(editor_state_t* state, const char* filename);
void editor_load_file(editor_state_t* state);
void editor_save_file(editor_state_t* state);
void editor_display(editor_state_t* state);
void editor_handle_key(editor_state_t* state, char key);
char editor_get_key(editor_state_t* state);
void editor_display_help_menu(editor_state_t* state);
bool editor_handle_help_menu(editor_state_t* state);
void terminal_clear(void);

// Initialize the editor state
void editor_init(editor_state_t* state, const char* filename) {
    // Clear all lines
    for (int i = 0; i < EDITOR_MAX_LINES; i++) {
        state->lines[i][0] = '\0';
    }
    
    // Initialize state
    state->num_lines = 1;  // Start with one empty line
    state->cursor_x = 0;
    state->cursor_y = 0;
    state->screen_offset = 0;
    state->modified = false;
    state->exit_requested = false;
    
    // Copy filename
    int i = 0;
    while (filename[i] && i < FS_MAX_NAME_LENGTH - 1) {
        state->filename[i] = filename[i];
        i++;
    }
    state->filename[i] = '\0';
}

// Update editor_load_file
void editor_load_file(editor_state_t* state) {
    // Find the file by path
    state->file_node = fs_find_node_by_path(state->filename);
    
    if (!state->file_node) {
        // File doesn't exist, start with empty buffer
        return;
    }
    
    // Check if it's a file (not a directory)
    if (state->file_node->type != FS_TYPE_FILE) {
        return;
    }
    
    // Read file content line by line
    int line = 0;
    int col = 0;
    const char* content = state->file_node->content;
    
    while (*content && line < EDITOR_MAX_LINES) {
        if (*content == '\n') {
            // End of line
            state->lines[line][col] = '\0';
            line++;
            col = 0;
        } else if (col < EDITOR_MAX_LINE_LENGTH - 1) {
            // Add character to current line
            state->lines[line][col] = *content;
            col++;
        }
        content++;
    }
    
    // Null-terminate the last line
    if (line < EDITOR_MAX_LINES) {
        state->lines[line][col] = '\0';
        state->num_lines = line + 1;
    } else {
        state->num_lines = EDITOR_MAX_LINES;
    }
}

// Update editor_save_file
void editor_save_file(editor_state_t* state) {
    // Create a buffer for the entire file content
    char content[FS_MAX_CONTENT_SIZE];
    size_t content_pos = 0;
    
    // Concatenate all lines
    for (int i = 0; i < state->num_lines; i++) {
        // Check if there's enough space
        if (content_pos + strlen(state->lines[i]) + 1 >= FS_MAX_CONTENT_SIZE) {
            break;
        }
        
        // Copy line content
        strcpy(&content[content_pos], state->lines[i]);
        content_pos += strlen(state->lines[i]);
        
        // Add newline (except for the last line)
        if (i < state->num_lines - 1) {
            content[content_pos++] = '\n';
        }
    }
    
    // Null-terminate the content
    content[content_pos] = '\0';
    
    // Get the directory and filename
    char* last_slash = strrchr(state->filename, '/');
    char filename[FS_MAX_NAME_LENGTH];
    fs_node_t* dir;
    
    if (last_slash) {
        // Extract directory path and filename
        *last_slash = '\0';  // Temporarily split the path
        dir = fs_find_node_by_path(state->filename);
        strcpy(filename, last_slash + 1);
        *last_slash = '/';   // Restore the path
        
        if (!dir) {
            return;  // Directory not found
        }
    } else {
        // No directory specified, use current directory
        dir = fs_get_cwd();
        strcpy(filename, state->filename);
    }
    
    // Check if file exists
    fs_node_t* file = fs_find_node(dir, filename);
    
    if (!file) {
        // Create new file
        file = fs_create_file(dir, filename);
        if (!file) {
            return;  // Failed to create file
        }
        state->file_node = file;
    }
    
    // Write content to file
    fs_write_file(file, content);
    
    // Mark as not modified
    state->modified = false;
}

// Get a key from the keyboard with special key handling
char editor_get_key(editor_state_t* state) {
    // Check if a key is available
    if ((inb(KEYBOARD_STATUS_PORT) & 1) == 0) {
        return 0; // No key available
    }
    
    // Read the scancode
    uint8_t scancode = inb(KEYBOARD_DATA_PORT);
    
    // Check for extended keys (prefixed with 0xE0)
    if (scancode == 0xE0) {
        // Wait for the next byte
        while ((inb(KEYBOARD_STATUS_PORT) & 1) == 0) {
            // Wait for the next byte
        }
        
        scancode = inb(KEYBOARD_DATA_PORT);
        
        // Translate extended scancodes to our special key values
        char special_key = 0;
        switch (scancode) {
            case 0x48: // Up arrow
                special_key = KEY_ARROW_UP;
                break;
            case 0x50: // Down arrow
                special_key = KEY_ARROW_DOWN;
                break;
            case 0x4B: // Left arrow
                special_key = KEY_ARROW_LEFT;
                break;
            case 0x4D: // Right arrow
                special_key = KEY_ARROW_RIGHT;
                break;
        }
        
        // If we recognized a special key, return it
        if (special_key) {
            return special_key;
        }
    }
    // Check for Ctrl+X (X is scancode 0x2D, Ctrl modifies it)
    else if (scancode == 0x2D) {
        // This is a simplified check - in a real implementation,
        // you'd track the state of modifier keys like Ctrl
        return KEY_CTRL_X;
    }
    // Regular key - convert to ASCII
    else if (scancode < 128) {
        // Use the keyboard mapping from your keyboard driver
        // This assumes your keyboard driver maintains the state of shift, etc.
        char key = 0;
        
        // Simple ASCII mapping for demonstration
        if (scancode < 128) {
            // Process the scancode to get the ASCII character
            // This is a simplified version - your actual implementation
            // would use your keyboard driver's mapping
            keyboard_process_scancode(scancode);
            key = keyboard_get_key();
        }
        
        return key;
    }
    
    return 0; // No recognized key
}

// Handle a key press in the editor
void editor_handle_key(editor_state_t* state, char key) {
    // Handle special keys
    if (key == '\b') {  // Backspace
        if (state->cursor_x > 0) {
            // Remove character at cursor position
            int line_len = strlen(state->lines[state->cursor_y]);
            for (int i = state->cursor_x - 1; i < line_len; i++) {
                state->lines[state->cursor_y][i] = state->lines[state->cursor_y][i + 1];
            }
            state->cursor_x--;
            state->modified = true;
        } else if (state->cursor_y > 0) {
            // Merge with previous line
            int prev_line_len = strlen(state->lines[state->cursor_y - 1]);
            int curr_line_len = strlen(state->lines[state->cursor_y]);
            
            // Check if there's room to merge
            if (prev_line_len + curr_line_len < EDITOR_MAX_LINE_LENGTH) {
                // Append current line to previous line
                strcat(state->lines[state->cursor_y - 1], state->lines[state->cursor_y]);
                
                // Move all lines up
                for (int i = state->cursor_y; i < state->num_lines - 1; i++) {
                    strcpy(state->lines[i], state->lines[i + 1]);
                }
                
                // Clear the last line if we're reducing line count
                if (state->num_lines > 1) {
                    state->num_lines--;
                    state->lines[state->num_lines][0] = '\0';
                }
                
                // Move cursor to end of previous line
                state->cursor_y--;
                state->cursor_x = prev_line_len;
                
                // Adjust screen offset if needed
                if (state->cursor_y < state->screen_offset) {
                    state->screen_offset = state->cursor_y;
                }
                
                state->modified = true;
            }
        }
    } else if (key == '\n') {  // Enter
        // Check if we have room for a new line
        if (state->num_lines < EDITOR_MAX_LINES) {
            // Move all lines down to make room for the new line
            for (int i = state->num_lines; i > state->cursor_y + 1; i--) {
                strcpy(state->lines[i], state->lines[i - 1]);
            }
            
            // Split the current line at cursor position
            state->lines[state->cursor_y + 1][0] = '\0';
            
            // Copy the part after cursor to the new line
            strcpy(state->lines[state->cursor_y + 1], &state->lines[state->cursor_y][state->cursor_x]);
            
            // Truncate the current line at cursor position
            state->lines[state->cursor_y][state->cursor_x] = '\0';
            
            // Increment line count
            state->num_lines++;
            
            // Move cursor to beginning of new line
            state->cursor_y++;
            state->cursor_x = 0;
            
            // Adjust screen offset if needed
            if (state->cursor_y >= state->screen_offset + VGA_HEIGHT - 2) {
                state->screen_offset = state->cursor_y - (VGA_HEIGHT - 3);
            }
            
            state->modified = true;
        }
    } else if (key == KEY_F1) {
        // Show help menu
        editor_display_help_menu(state);
    } else if (key == KEY_CTRL_X) {
        // Set exit flag
        state->exit_requested = true;
    } 
    // Handle arrow keys - make sure these are defined with values outside ASCII range
    else if (key == KEY_ARROW_UP) {
        // Move cursor up
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
        // Move cursor down
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
        // Move cursor left
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
        // Move cursor right
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
    } 
    // Handle printable ASCII characters
    else if (key >= 32 && key <= 126) {
        // Check if there's room in the line
        int line_len = strlen(state->lines[state->cursor_y]);
        if (line_len < EDITOR_MAX_LINE_LENGTH - 1) {
            // Make room for the new character
            for (int i = line_len; i >= state->cursor_x; i--) {
                state->lines[state->cursor_y][i + 1] = state->lines[state->cursor_y][i];
            }
            
            // Insert the character
            state->lines[state->cursor_y][state->cursor_x] = key;
            state->cursor_x++;
            state->modified = true;
        }
    }
}

// Display the editor content
void editor_display(editor_state_t* state) {
    // Save current terminal color
    uint8_t old_color = terminal_color;
    
    // Display header
    terminal_setcolor(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    terminal_putentryat(' ', terminal_color, 0, 0);
    terminal_writestring(" PIA Editor - ");
    terminal_writestring(state->filename);
    if (state->modified) {
        terminal_writestring(" [modified]");
    }
    
    // Fill the rest of the header line
    for (int i = 0; i < VGA_WIDTH - strlen(" PIA Editor - ") - strlen(state->filename) - 
                    (state->modified ? strlen(" [modified]") : 0); i++) {
        terminal_putentryat(' ', terminal_color, 
                          strlen(" PIA Editor - ") + strlen(state->filename) + 
                          (state->modified ? strlen(" [modified]") : 0) + i, 0);
    }
    
    // Display editor content
    terminal_setcolor(vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK));
    
    // Calculate visible lines
    int visible_lines = VGA_HEIGHT - 2;  // Header and footer
    
    // Display each visible line
    for (int i = 0; i < visible_lines; i++) {
        int line_num = state->screen_offset + i;
        
        // Clear the line
        for (int j = 0; j < VGA_WIDTH; j++) {
            terminal_putentryat(' ', terminal_color, j, i + 1);
        }
        
        // Display line content if it exists
        if (line_num < state->num_lines) {
            int line_len = strlen(state->lines[line_num]);
            for (int j = 0; j < line_len && j < VGA_WIDTH; j++) {
                terminal_putentryat(state->lines[line_num][j], terminal_color, j, i + 1);
            }
        }
    }
    
    // Display footer
    terminal_setcolor(vga_entry_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY));
    terminal_putentryat(' ', terminal_color, 0, VGA_HEIGHT - 1);
    terminal_writestring(" F1: Open Help Menu");

    // Fill the rest of the footer line
    for (int i = 0; i < VGA_WIDTH - strlen(" F1: Open Help Menu"); i++) {
        terminal_putentryat(' ', terminal_color, strlen(" F1: Open Help Menu") + i, VGA_HEIGHT - 1);
    }

    // Position cursor
    update_cursor(state->cursor_y - state->screen_offset + 1, state->cursor_x);
    
    // Restore original color
    terminal_setcolor(old_color);
}

// Display help menu
void editor_display_help_menu(editor_state_t* state) {
    // Save current terminal color
    uint8_t old_color = terminal_color;
    
    // Clear a portion of the screen for the menu
    int menu_start_row = (VGA_HEIGHT - 8) / 2;
    int menu_width = 40;
    int menu_start_col = (VGA_WIDTH - menu_width) / 2;
    
    // Draw menu box
    for (int row = menu_start_row; row < menu_start_row + 8; row++) {
        for (int col = menu_start_col; col < menu_start_col + menu_width; col++) {
            if (row == menu_start_row || row == menu_start_row + 7 || 
                col == menu_start_col || col == menu_start_col + menu_width - 1) {
                // Draw border
                terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
                terminal_putentryat(' ', terminal_color, col, row);
            } else {
                // Fill background
                terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
                terminal_putentryat(' ', terminal_color, col, row);
            }
        }
    }
    
    // Draw menu title
    terminal_setcolor(vga_entry_color(VGA_COLOR_BROWN, VGA_COLOR_BLUE));
    const char* title = " PIA Editor Help ";
    int title_col = menu_start_col + (menu_width - strlen(title)) / 2;
    for (int i = 0; i < strlen(title); i++) {
        terminal_putentryat(title[i], terminal_color, title_col + i, menu_start_row);
    }

    // Draw menu options
    terminal_setcolor(vga_entry_color(VGA_COLOR_WHITE, VGA_COLOR_BLUE));
    const char* option1 = "1. Save and Exit";
    const char* option2 = "2. Discard changes and Exit";
    const char* option3 = "3. Save";
    const char* option4 = "4. Return to Editor";

    // For option1, increment the column position for each character
    for (int i = 0; i < strlen(option1); i++) {
        terminal_putentryat(option1[i], terminal_color, menu_start_col + 2 + i, menu_start_row + 2);
    }

    // For option2, increment the column position for each character
    for (int i = 0; i < strlen(option2); i++) {
        terminal_putentryat(option2[i], terminal_color, menu_start_col + 2 + i, menu_start_row + 3);
    }

    // For option3, increment the column position for each character
    for (int i = 0; i < strlen(option3); i++) {
        terminal_putentryat(option3[i], terminal_color, menu_start_col + 2 + i, menu_start_row + 4);
    }

    // For option4, increment the column position for each character
    for (int i = 0; i < strlen(option4); i++) {
        terminal_putentryat(option4[i], terminal_color, menu_start_col + 2 + i, menu_start_row + 5);
    }
    
    // Restore original color
    terminal_setcolor(old_color);
}

// Handle help menu
bool editor_handle_help_menu(editor_state_t* state) {
    editor_display_help_menu(state);
    
    // Wait for a key press
    while (1) {
        keyboard_poll();
        char key = keyboard_get_key();
        
        if (key != 0) {
            switch (key) {
                case '1':  // Save and Exit
                    editor_save_file(state);
                    return false;  // Exit editor
                
                case '2':  // Discard changes and Exit
                    return false;  // Exit editor without saving
                
                case '3':  // Save
                    editor_save_file(state);
                    return true;   // Continue editing
                
                case '4':  // Return to Editor
                case 27:   // ESC key
                    return true;   // Continue editing
            }
        }
    }
}

// Main editor function
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
        
        // Check for special keys first (arrow keys, F1)
        if ((inb(KEYBOARD_STATUS_PORT) & 1) != 0) {
            uint8_t scancode = inb(KEYBOARD_DATA_PORT);
            
            // Check for F1 key
            if (scancode == 0x3B) {  // F1 key
                // Display help menu and get result
                bool continue_editing = editor_handle_help_menu(&state);
                if (!continue_editing) {
                    running = false;
                } else {
                    // Redraw the editor
                    editor_display(&state);
                }
                continue;
            }
            
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
                        {
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
        }
        
        // Process regular keys from the keyboard buffer
        char key = editor_get_key(&state);
        
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
        
        // Check exit flag
        if (state.exit_requested) {
            running = false;
        }
    }
    
    // Clear screen when exiting
    terminal_clear();
    terminal_writestring("File saved: ");
    terminal_writestring(state.filename);
    terminal_writestring("\n");
}
