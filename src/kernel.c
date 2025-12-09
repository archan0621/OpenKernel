#include <stdint.h>
#include "drivers/video/video.h"

#define MB2_MAGIC 0x36d76289

static void hlt_loop(void) {
    for(;;) __asm__ __volatile__("hlt");
}

void kernel_main(uint32_t magic, void* mbinfo) {
    if (magic != MB2_MAGIC)
        hlt_loop();

    video_init(mbinfo);
    video_print_char();

    hlt_loop();
}