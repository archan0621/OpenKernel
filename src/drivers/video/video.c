#include "drivers/video/video.h"
#include "font/font8x16.h"

static struct multiboot_tag_framebuffer* g_fb = 0;
static int g_use_vga_text = 0;

static struct multiboot_tag_framebuffer* find_fb_tag(void* mbinfo) {
    uint8_t* p = (uint8_t*)mbinfo + 8; // skip total_size, reserved

    while (1) {
        struct multiboot_tag* tag = (struct multiboot_tag*)p;
        if (tag->type == MB2_TAG_TYPE_END)
            return 0;

        if (tag->type == MB2_TAG_TYPE_FRAMEBUFFER)
            return (struct multiboot_tag_framebuffer*)tag;

        p += (tag->size + 7) & ~7; // align 8
    }
}

void video_init(void* mbinfo) {
    struct multiboot_tag_framebuffer* fb = find_fb_tag(mbinfo);
    
    if (fb && fb->framebuffer_bpp == 32) {
        g_fb = fb;
        g_use_vga_text = 0;
    } else {
        g_fb = 0;
        g_use_vga_text = 1;
    }
}

// Clear entire screen with background color (black for now)
void video_clear_screen(void) {
    if (g_use_vga_text) {
        // VGA text mode (bios)
        volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
        for (int i = 0; i < 80 * 25; i++) {
            vga[i] = 0x0000; // black background
        }
    } else {
        // Framebuffer mode (UEFI)
        if (!g_fb || g_fb->framebuffer_bpp != 32)
            return;

        uint32_t width  = g_fb->framebuffer_width;
        uint32_t height = g_fb->framebuffer_height;
        uint32_t pitch  = g_fb->framebuffer_pitch;
        uint32_t* buffer = (uint32_t*)(uintptr_t)g_fb->framebuffer_addr;

        for (uint32_t y = 0; y < height; y++) {
            uint32_t* row = (uint32_t*)((uint8_t*)buffer + pitch * y);
            for (uint32_t x = 0; x < width; x++) {
                row[x] = 0xFF000000; // black
            }
        }
    }
}

// Draw a single character at character coordinates
void video_draw_char(int cx, int cy, char c) {
    if (g_use_vga_text) {
        // VGA text mode (bios)
        if (cx < 0 || cx >= 80 || cy < 0 || cy >= 25)
            return;

        volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
        int pos = cy * 80 + cx;
        vga[pos] = 0x0700 | (uint8_t)c; // black background, light gray text
    } else {
        // Framebuffer mode (UEFI)
        if (!g_fb || g_fb->framebuffer_bpp != 32)
            return;

        // Check character range
        if (c < FONT8X16_FIRST || c > FONT8X16_LAST)
            return;

        uint32_t width  = g_fb->framebuffer_width;
        uint32_t height = g_fb->framebuffer_height;
        uint32_t pitch  = g_fb->framebuffer_pitch;
        uint32_t* buffer = (uint32_t*)(uintptr_t)g_fb->framebuffer_addr;

        // Convert character coordinates to pixel coordinates
        int px = cx * FONT8X16_WIDTH;
        int py = cy * FONT8X16_HEIGHT;

        // Check screen bounds
        if (px < 0 || px + FONT8X16_WIDTH > (int)width ||
            py < 0 || py + FONT8X16_HEIGHT > (int)height)
            return;

        // Get font data
        const uint8_t* font_data = font8x16[c - FONT8X16_FIRST];
        uint32_t color = 0xFFFFFFFF; // white

        // Draw font
        for (int row = 0; row < FONT8X16_HEIGHT; row++) {
            uint8_t bits = font_data[row];
            for (int col = 0; col < FONT8X16_WIDTH; col++) {
                if (bits & (1 << (7 - col))) {
                    int x = px + col;
                    int y = py + row;
                    uint32_t* pixel = (uint32_t*)((uint8_t*)buffer + (y * pitch) + (x * 4));
                    *pixel = color;
                }
            }
        }
    }
}

// Scroll screen up by one line
void video_scroll_up(void) {
    if (g_use_vga_text) {
        // VGA text mode: copy lines 1-24 to 0-23, clear line 24
        volatile uint16_t* vga = (volatile uint16_t*)0xB8000;
        for (int y = 0; y < 24; y++) {
            for (int x = 0; x < 80; x++) {
                vga[y * 80 + x] = vga[(y + 1) * 80 + x];
            }
        }
        // Clear last line
        for (int x = 0; x < 80; x++) {
            vga[24 * 80 + x] = 0x0000;
        }
    } else {
        // Framebuffer mode
        if (!g_fb || g_fb->framebuffer_bpp != 32)
            return;

        uint32_t width  = g_fb->framebuffer_width;
        uint32_t height = g_fb->framebuffer_height;
        uint32_t pitch  = g_fb->framebuffer_pitch;
        uint32_t* buffer = (uint32_t*)(uintptr_t)g_fb->framebuffer_addr;
        uint32_t line_height = FONT8X16_HEIGHT;
        uint32_t chars_per_line = width / FONT8X16_WIDTH;
        uint32_t lines_per_screen = height / line_height;

        // Copy lines 1 to (lines_per_screen-1) to lines 0 to (lines_per_screen-2)
        for (uint32_t line = 0; line < lines_per_screen - 1; line++) {
            uint32_t src_y = (line + 1) * line_height;
            uint32_t dst_y = line * line_height;
            for (uint32_t row = 0; row < line_height; row++) {
                uint32_t* src_row = (uint32_t*)((uint8_t*)buffer + pitch * (src_y + row));
                uint32_t* dst_row = (uint32_t*)((uint8_t*)buffer + pitch * (dst_y + row));
                for (uint32_t x = 0; x < width; x++) {
                    dst_row[x] = src_row[x];
                }
            }
        }
        // Clear last line
        uint32_t last_line_y = (lines_per_screen - 1) * line_height;
        for (uint32_t row = 0; row < line_height; row++) {
            uint32_t* last_row = (uint32_t*)((uint8_t*)buffer + pitch * (last_line_y + row));
            for (uint32_t x = 0; x < width; x++) {
                last_row[x] = 0xFF000000; // black
            }
        }
    }
}
