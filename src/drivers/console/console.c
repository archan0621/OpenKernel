#include "drivers/console/console.h"
#include "drivers/video/video.h"

// Console dimensions
#define CONSOLE_WIDTH_VGA  80
#define CONSOLE_HEIGHT_VGA 25

// Current cursor position
static int g_cursor_x = 0;
static int g_cursor_y = 0;
static int g_console_width = CONSOLE_WIDTH_VGA;
static int g_console_height = CONSOLE_HEIGHT_VGA;

// Initialize console
void console_init(void* mbinfo) {
    video_init(mbinfo);
    
    // Calculate console dimensions based on video mode
    // For now, assume VGA text mode dimensions
    // TODO: Calculate framebuffer dimensions if needed
    g_cursor_x = 0;
    g_cursor_y = 0;
    g_console_width = CONSOLE_WIDTH_VGA;
    g_console_height = CONSOLE_HEIGHT_VGA;
}

// Clear screen
void console_clear(void) {
    video_clear_screen();
    g_cursor_x = 0;
    g_cursor_y = 0;
}

// Put a single character
void console_putc(char c) {
    // Check if cursor is out of bounds
    if (g_cursor_y >= g_console_height)
        return;
    
    if (c == '\n') {
        // Newline: move to next line
        g_cursor_x = 0;
        g_cursor_y++;
    } else if (c == '\r') {
        // Carriage return: move to beginning of line
        g_cursor_x = 0;
    } else if (c == '\t') {
        // Tab: move to next tab stop (every 4 spaces)
        g_cursor_x = (g_cursor_x + 4) & ~3;
        if (g_cursor_x >= g_console_width) {
            g_cursor_x = 0;
            g_cursor_y++;
        }
    } else {
        // Normal character
        if (g_cursor_x < g_console_width && g_cursor_y < g_console_height) {
            video_draw_char(g_cursor_x, g_cursor_y, c);
        }
        g_cursor_x++;
        
        // Wrap to next line if needed
        if (g_cursor_x >= g_console_width) {
            g_cursor_x = 0;
            g_cursor_y++;
        }
    }
}

// Put a string
void console_puts(const char* s) {
    while (*s) {
        console_putc(*s);
        s++;
    }
}

