#include "mem/mmap.h"
#include "drivers/console/console.h"

#include <stdint.h>
#include <stddef.h>

static struct multiboot_tag_mmap* find_mmap_tag(void* mbinfo) {
    // Multiboot2 info starts with:
    //   u32 total_size, u32 reserved
    uint8_t* p = (uint8_t*)mbinfo + 8;

    while (1) {
        struct multiboot_tag* tag = (struct multiboot_tag*)p;

        if (tag->type == MB2_TAG_TYPE_END) return NULL;
        if (tag->type == MB2_TAG_TYPE_MMAP) return (struct multiboot_tag_mmap*)tag;

        // tags are 8-byte aligned
        p += (tag->size + 7) & ~7u;
    }
}

static void console_putu32(uint32_t v) {
    char buf[11];
    int idx = 0;

    if (v == 0) {
        console_putc('0');
        return;
    }

    while (v > 0 && idx < 10) {
        buf[idx++] = (char)('0' + (v % 10));
        v /= 10;
    }

    while (idx--) {
        console_putc(buf[idx]);
    }
}

static void console_puthex64(uint64_t v) {
    console_puts("0x");
    for (int i = 60; i >= 0; i -= 4) {
        uint8_t nibble = (uint8_t)((v >> i) & 0xFULL);
        char c = (nibble < 10) ? (char)('0' + nibble) : (char)('a' + (nibble - 10));
        console_putc(c);
    }
}

void mmap_dump(uint32_t magic, void* mbinfo) {
    if (magic != MB2_MAGIC) {
        console_puts("[MMAP] invalid magic\n");
        return;
    }

    struct multiboot_tag_mmap* mm = find_mmap_tag(mbinfo);
    if (!mm) {
        console_puts("[MMAP] no mmap tag\n");
        return;
    }

    console_puts("[MMAP] found\n");

    uint8_t* start = (uint8_t*)mm + sizeof(*mm);
    uint8_t* end   = (uint8_t*)mm + mm->size;

    for (uint8_t* p = start; p < end; p += mm->entry_size) {
        struct multiboot_mmap_entry* e = (struct multiboot_mmap_entry*)p;

        console_puts("  type=");
        console_putu32(e->type);

        console_puts(" addr=");
        console_puthex64(e->addr);

        console_puts(" len=");
        console_puthex64(e->len);

        if (e->type == 1) console_puts(" (usable)");
        console_putc('\n');
    }
}