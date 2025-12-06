#include "video.h"

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

static const uint8_t FONT_H[16] = {
    0b10001000,
    0b10001000,
    0b10001000,
    0b11111000,
    0b10001000,
    0b10001000,
    0b10001000,
    0b10001000,
    0,0,0,0,0,0,0,0
};


static void vga_text_mode_print_h(void) {
    volatile uint16_t* vga = (volatile uint16_t*)0xB8000;

    for (int i = 0; i < 80 * 25; i++) {
        vga[i] = 0x0000;
    }

    vga[0] = 0x0748;
}

static void framebuffer_print_h(struct multiboot_tag_framebuffer* fb) {
    if (fb->framebuffer_bpp != 32)
        return;

    uint32_t width  = fb->framebuffer_width;
    uint32_t height = fb->framebuffer_height;
    uint32_t pitch  = fb->framebuffer_pitch;

    uint32_t* buffer = (uint32_t*)(uintptr_t)fb->framebuffer_addr;

    // init screen black
    for (uint32_t y = 0; y < height; y++) {
        uint32_t* row = (uint32_t*)((uint8_t*)buffer + pitch * y);
        for (uint32_t x = 0; x < width; x++) {
            row[x] = 0xFF000000; // black
        }
    }

    // left top print h
    uint32_t color = 0xFFFFFFFF; // white
    for (int row = 0; row < 16; row++) {
        uint8_t bits = FONT_H[row];
        for (int col = 0; col < 8; col++) {
            if (bits & (1 << (7 - col))) {
                uint32_t* pixel = (uint32_t*)((uint8_t*)buffer + (row * pitch) + (col * 4));
                *pixel = color;
            }
        }
    }
}

void video_print_char() {
    if (g_use_vga_text) {
        // VGA text mode (bios)
        vga_text_mode_print_h();
    } else {
        // Framebuffer mode (UEFI)
        framebuffer_print_h(g_fb);
    }
}
