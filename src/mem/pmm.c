#include "mem/pmm.h"
#include "mem/mmap.h"
#include "drivers/console/console.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define MB2_MAGIC 0x36d76289
#define MB2_TAG_TYPE_END   0
#define MB2_TAG_TYPE_MMAP  6

struct multiboot_tag {
    uint32_t type;
    uint32_t size;
};

struct multiboot_tag_mmap {
    uint32_t type;
    uint32_t size;
    uint32_t entry_size;
    uint32_t entry_version;
};

struct multiboot_mmap_entry {
    uint64_t addr;
    uint64_t len;
    uint32_t type;
    uint32_t zero;
};

static uint32_t total_pages = 0;
static uint32_t free_pages = 0;
static uint8_t g_bitmap[128 * 1024];
static uint8_t* bitmap = NULL;
static uint32_t bitmap_size = 0;
static uint64_t memory_start = 0;
static uint64_t memory_end = 0;

extern char kernel_start;
extern char kernel_end;
static inline void bitmap_set(uint32_t page_idx) {
    if (!bitmap || page_idx >= total_pages) return;
    uint32_t byte_idx = page_idx / 8;
    uint32_t bit_idx = page_idx % 8;
    if (byte_idx >= bitmap_size) return;
    bitmap[byte_idx] |= (1u << bit_idx);
}

static inline void bitmap_clear(uint32_t page_idx) {
    if (!bitmap || page_idx >= total_pages) return;
    uint32_t byte_idx = page_idx / 8;
    uint32_t bit_idx = page_idx % 8;
    if (byte_idx >= bitmap_size) return;
    bitmap[byte_idx] &= ~(1u << bit_idx);
}

static inline bool bitmap_get(uint32_t page_idx) {
    if (!bitmap || page_idx >= total_pages) return true;
    uint32_t byte_idx = page_idx / 8;
    uint32_t bit_idx = page_idx % 8;
    if (byte_idx >= bitmap_size) return true;
    return (bitmap[byte_idx] & (1u << bit_idx)) != 0;
}

static struct multiboot_tag_mmap* find_mmap_tag(void* mbinfo) {
    uint8_t* p = (uint8_t*)mbinfo + 8;

    while (1) {
        struct multiboot_tag* tag = (struct multiboot_tag*)p;

        if (tag->type == MB2_TAG_TYPE_END) return NULL;
        if (tag->type == MB2_TAG_TYPE_MMAP) return (struct multiboot_tag_mmap*)tag;

        p += (tag->size + 7) & ~7u;
    }
}

void pmm_init(uint32_t magic, void* mbinfo) {
    console_puts("[PMM] Starting initialization...\n");
    
    if (magic != MB2_MAGIC) {
        console_puts("[PMM] Invalid magic\n");
        return;
    }

    struct multiboot_tag_mmap* mm = find_mmap_tag(mbinfo);
    if (!mm) {
        console_puts("[PMM] No memory map tag found\n");
        return;
    }
    
    console_puts("[PMM] Memory map found\n");

    uint8_t* start = (uint8_t*)mm + sizeof(*mm);
    uint8_t* end = (uint8_t*)mm + mm->size;

    uint64_t max_addr = 0;
    uint64_t min_addr = 0xFFFFFFFFFFFFFFFFULL;

    console_puts("[PMM] Scanning memory map...\n");
    
    uint32_t entry_size = mm->entry_size;
    if (entry_size == 0 || entry_size > 64) {
        console_puts("[PMM] Invalid entry size\n");
        return;
    }
    
    uint8_t* p = start;
    uint32_t entry_count = 0;
    
    while (p < end && entry_count < 64) {
        if (p + sizeof(struct multiboot_mmap_entry) > end) {
            break;
        }
        
        struct multiboot_mmap_entry* e = (struct multiboot_mmap_entry*)p;

        if (e->type == 1) {
            uint32_t addr_low = (uint32_t)e->addr;
            uint32_t addr_high = (uint32_t)(e->addr >> 32);
            uint32_t len_low = (uint32_t)e->len;
            uint32_t len_high = (uint32_t)(e->len >> 32);
            
            // Only process 32-bit addresses
            if (addr_high == 0 && len_high == 0) {
                uint32_t entry_start_32 = addr_low;
                uint32_t entry_end_32 = addr_low + len_low;
                uint64_t entry_start = (uint64_t)entry_start_32;
                uint64_t entry_end = (uint64_t)entry_end_32;

                if (min_addr == 0xFFFFFFFFFFFFFFFFULL || entry_start < min_addr) {
                    min_addr = entry_start;
                }
                if (entry_end > max_addr) {
                    max_addr = entry_end;
                }
            }
        }
        
        p += entry_size;
        entry_count++;
    }

    console_puts("[PMM] Memory scan complete\n");
    
    if (min_addr == 0xFFFFFFFFFFFFFFFFULL || max_addr == 0) {
        console_puts("[PMM] No usable memory found\n");
        return;
    }

    console_puts("[PMM] Aligning memory...\n");
    
    uint32_t min_addr_32 = (uint32_t)min_addr;
    uint32_t max_addr_32 = (uint32_t)max_addr;
    uint32_t page_mask_32 = ~(PAGE_SIZE - 1);
    
    uint32_t mem_start_32 = (min_addr_32 + PAGE_SIZE - 1) & page_mask_32;
    
    // Exclude 0~1MB from PMM management (standard approach)
    const uint32_t PMM_MIN_ADDR = 0x100000;
    if (mem_start_32 < PMM_MIN_ADDR) {
        mem_start_32 = PMM_MIN_ADDR;
    }
    
    uint32_t mem_end_32 = max_addr_32 & page_mask_32;

    if (mem_end_32 <= mem_start_32) {
        console_puts("[PMM] Invalid memory range\n");
        return;
    }
    
    memory_start = (uint64_t)mem_start_32;
    memory_end = (uint64_t)mem_end_32;

    uint32_t mem_diff_32 = mem_end_32 - mem_start_32;
    total_pages = mem_diff_32 >> 12;
    
    if (total_pages > 0x100000) {
        total_pages = 0x100000;
    }
    
    console_puts("[PMM] Page count calculated\n");
    
    bitmap_size = (total_pages + 7) / 8;
    
    console_puts("[PMM] Memory range calculated\n");

    console_puts("[PMM] Calculating kernel region...\n");
    
    uint32_t kernel_start_addr_32 = (uint32_t)&kernel_start;
    uint32_t kernel_end_addr_32 = (uint32_t)&kernel_end;
    uint64_t kernel_start_addr = (uint64_t)kernel_start_addr_32;
    uint64_t kernel_end_addr = (uint64_t)kernel_end_addr_32;
    
    uint64_t kernel_start_page = 0;
    uint64_t kernel_end_page = 0;
    
    if (kernel_start_addr >= memory_start && kernel_start_addr < memory_end) {
        uint64_t kernel_offset = kernel_start_addr - memory_start;
        kernel_start_page = kernel_offset >> 12;
        
        uint64_t kernel_end_offset = kernel_end_addr - memory_start;
        kernel_end_page = (kernel_end_offset + PAGE_SIZE - 1) >> 12;
        if (kernel_end_page > (uint64_t)total_pages) {
            kernel_end_page = total_pages;
        }
    }
    
    console_puts("[PMM] Kernel region calculated\n");

    bitmap = g_bitmap;
    
    uint32_t max_bitmap_size = sizeof(g_bitmap);
    if (bitmap_size > max_bitmap_size) {
        console_puts("[PMM] Bitmap size exceeds static allocation\n");
        bitmap_size = max_bitmap_size;
        total_pages = max_bitmap_size * 8;
    }

    console_puts("[PMM] Bitmap initialized (static allocation)\n");

    // Initialize bitmap to 0xFF (all pages marked as used)
    // BSS initializes to 0, but 0 means free in our semantics, so we need to set all to used first
    if (bitmap != NULL && bitmap_size > 0) {
        uint8_t* bmp = bitmap;
        uint32_t size = bitmap_size;
        
        __asm__ __volatile__ (
            "cld\n\t"
            "rep stosb\n\t"
            : "+c" (size), "+D" (bmp)
            : "a" (0xFF)
            : "memory"
        );
    }
    console_puts("[PMM] Bitmap set to all used (0xFF)\n");

    console_puts("[PMM] Marking free pages...\n");
    
    free_pages = 0;
    p = start;
    entry_count = 0;
    uint32_t memory_start_32 = (uint32_t)memory_start;
    uint32_t memory_end_32 = (uint32_t)memory_end;
    
    while (p < end && entry_count < 64) {
        if (p + sizeof(struct multiboot_mmap_entry) > end) {
            break;
        }
        
        struct multiboot_mmap_entry* e = (struct multiboot_mmap_entry*)p;

        if (e->type == 1) {
            uint32_t addr_low = (uint32_t)e->addr;
            uint32_t addr_high = (uint32_t)(e->addr >> 32);
            uint32_t len_low = (uint32_t)e->len;
            uint32_t len_high = (uint32_t)(e->len >> 32);
            
            if (addr_high == 0 && len_high == 0) {
                uint32_t entry_start_32 = addr_low;
                uint32_t entry_end_32 = addr_low + len_low;
                uint32_t entry_start_aligned = (entry_start_32 + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
                uint32_t entry_end_aligned = entry_end_32 & ~(PAGE_SIZE - 1);

                if (entry_end_aligned > entry_start_aligned) {
                    if (entry_start_aligned < memory_start_32) entry_start_aligned = memory_start_32;
                    if (entry_end_aligned > memory_end_32) entry_end_aligned = memory_end_32;

                    uint32_t page_start_offset = entry_start_aligned - memory_start_32;
                    uint32_t page_end_offset = entry_end_aligned - memory_start_32;
                    uint32_t page_start = page_start_offset >> 12;
                    uint32_t page_end = page_end_offset >> 12;

                    for (uint32_t page_idx = page_start; page_idx < page_end && page_idx < total_pages; page_idx++) {
                        if (kernel_start_page > 0 && kernel_end_page > 0 && 
                            page_idx >= (uint32_t)kernel_start_page && page_idx < (uint32_t)kernel_end_page) {
                            continue;
                        }

                        if (bitmap != NULL) {
                            bitmap_clear(page_idx);
                            free_pages++;
                        }
                    }
                }
            }
        }
        
        p += entry_size;
        entry_count++;
    }

    console_puts("[PMM] Initialized: ");
    if (total_pages > 0) {
        uint32_t t = total_pages;
        char buf[12];
        int idx = 0;
        while (t > 0 && idx < 11) {
            buf[idx++] = (char)('0' + (t % 10));
            t /= 10;
        }
        while (idx--) console_putc(buf[idx]);
    }
    console_puts(" total pages, ");
    if (free_pages > 0) {
        uint32_t f = free_pages;
        char buf[12];
        int idx = 0;
        while (f > 0 && idx < 11) {
            buf[idx++] = (char)('0' + (f % 10));
            f /= 10;
        }
        while (idx--) console_putc(buf[idx]);
    }
    console_puts(" free pages\n");
}

void* pmm_alloc_page(void) {
    if (!bitmap || free_pages == 0) {
        return NULL;
    }

    for (uint32_t i = 0; i < total_pages; i++) {
        if (!bitmap_get(i)) {
            bitmap_set(i);
            free_pages--;
            return (void*)(memory_start + (uint64_t)i * PAGE_SIZE);
        }
    }

    return NULL;
}

void pmm_free_page(void* page) {
    if (!bitmap || !page) {
        return;
    }

    uint64_t page_addr = (uint64_t)page;
    
    if (page_addr < memory_start || page_addr >= memory_end) {
        return;
    }

    if ((page_addr & 0xFFF) != 0) {
        return;
    }

    uint32_t page_offset = (uint32_t)(page_addr - memory_start);
    uint32_t page_idx = page_offset >> 12;
    
    if (page_idx >= total_pages) {
        return;
    }

    if (!bitmap_get(page_idx)) {
        return;
    }

    bitmap_clear(page_idx);
    free_pages++;
}

uint32_t pmm_total_pages(void) {
    return total_pages;
}

uint32_t pmm_free_pages(void) {
    return free_pages;
}
