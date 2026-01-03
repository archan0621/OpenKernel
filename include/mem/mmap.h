#pragma once
#include <stdint.h>

// Multiboot2 bootloader magic (GRUB가 EAX에 넣어줌)
#define MB2_MAGIC 0x36d76289

// Multiboot2 tag types
#define MB2_TAG_TYPE_END   0
#define MB2_TAG_TYPE_MMAP  6

// Multiboot2 tag header (common structure)
struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

// MMAP tag header (tag type 6)
struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
    // entries follow
};

// One mmap entry
struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;   // 1 = available
    uint32_t zero;
};

void mmap_dump(uint32_t magic, void* mbinfo);