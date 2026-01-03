#pragma once

#include <stdint.h>

#define MB2_TAG_TYPE_END          0
#define MB2_TAG_TYPE_FRAMEBUFFER  8

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_framebuffer {
    uint32_t type;
    uint32_t size;
    uint64_t framebuffer_addr;
    uint32_t framebuffer_pitch;
    uint32_t framebuffer_width;
    uint32_t framebuffer_height;
    uint8_t  framebuffer_bpp;
    uint8_t  framebuffer_type;
    uint16_t reserved;
};

void video_init(void* mbinfo);
void video_clear_screen(void);
void video_draw_char(int cx, int cy, char c);
void video_scroll_up(void);

