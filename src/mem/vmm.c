#include "mem/vmm.h"
#include "mem/pmm.h"
#include "drivers/console/console.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Simple memset implementation (string.h may not be available)
static void* memset(void* s, int c, size_t n) {
    uint8_t* p = (uint8_t*)s;
    for (size_t i = 0; i < n; i++) {
        p[i] = (uint8_t)c;
    }
    return s;
}

// Currently active page directory
static void* current_page_dir = NULL;

// Page directory/table entry type
typedef uint32_t page_entry_t;

// Page directory/table type (used as pointer)
typedef page_entry_t* page_table_t;
typedef page_entry_t* page_dir_t;

// Assembly function to set page directory in CR3 register
extern void vmm_flush(void* page_dir);

// Extract physical address from page entry
static inline void* entry_get_addr(page_entry_t entry) {
    return (void*)(entry & 0xFFFFF000);
}

// Create page entry
static inline page_entry_t entry_create(void* phys_addr, uint32_t flags) {
    return ((uint32_t)phys_addr & 0xFFFFF000) | (flags & 0xFFF);
}

// Check if page entry is present
static inline bool entry_is_present(page_entry_t entry) {
    return (entry & VMM_PRESENT) != 0;
}

// Allocate page table (uses PMM)
void* vmm_alloc_page_table(void) {
    void* page = pmm_alloc_page();
    if (page) {
        // Verify 4KB alignment
        if ((uint32_t)page & 0xFFF) {
            return NULL;
        }
        
        // Initialize page to zero
        memset(page, 0, VMM_PAGE_SIZE);
    }
    return page;
}

// Free page table
void vmm_free_page_table(void* page_table) {
    if (page_table) {
        pmm_free_page(page_table);
    }
}

// Create page directory
void* vmm_create_page_dir(void) {
    void* page_dir = vmm_alloc_page_table();
    if (!page_dir) {
        console_puts("[VMM] Failed to allocate page directory\n");
        return NULL;
    }
    
    // All entries are initialized to zero (already set by memset)
    // First page table must always exist (for identity mapping)
    
    return page_dir;
}

// Destroy page directory
void vmm_destroy_page_dir(void* page_dir) {
    if (!page_dir) return;
    
    page_dir_t dir = (page_dir_t)page_dir;
    
    // Free all page tables
    for (uint32_t i = 0; i < VMM_PAGE_DIR_ENTRIES; i++) {
        if (entry_is_present(dir[i])) {
            void* page_table = entry_get_addr(dir[i]);
            vmm_free_page_table(page_table);
        }
    }
    
    // Free the page directory itself
    vmm_free_page_table(page_dir);
    
    console_puts("[VMM] Page directory destroyed\n");
}

// Map page
bool vmm_map_page(void* page_dir, void* virt_addr, void* phys_addr, uint32_t flags) {
    if (!page_dir || !virt_addr || !phys_addr) {
        return false;
    }
    
    // Verify virtual address alignment
    if (((uint32_t)virt_addr & 0xFFF) != 0 || ((uint32_t)phys_addr & 0xFFF) != 0) {
        return false;
    }
    
    page_dir_t dir = (page_dir_t)page_dir;
    uint32_t dir_idx = VMM_PAGE_DIR_INDEX(virt_addr);
    uint32_t table_idx = VMM_PAGE_TABLE_INDEX(virt_addr);
    
    if (dir_idx >= VMM_PAGE_DIR_ENTRIES || table_idx >= VMM_PAGE_TABLE_ENTRIES) {
        return false;
    }
    
    // Check page directory entry
    page_entry_t dir_entry = dir[dir_idx];
    
    page_table_t table;
    if (!entry_is_present(dir_entry)) {
        // Create page table if it doesn't exist
        // pmm_alloc_page() returns physical address (before paging enabled, virtual = physical)
        void* new_table_phys = vmm_alloc_page_table();
        if (!new_table_phys) {
            return false;
        }
        
        // Store physical address in page directory entry (physical address needed for CR3)
        // entry_create masks lower 12 bits, so 4KB alignment is verified
        dir[dir_idx] = entry_create(new_table_phys, VMM_PRESENT | VMM_WRITABLE | VMM_USER);
        
        // Before paging is enabled, we can access physical address directly
        table = (page_table_t)new_table_phys;
    } else {
        // Use existing page table
        table = (page_table_t)entry_get_addr(dir_entry);
    }
    
    // Set page table entry
    if (entry_is_present(table[table_idx])) {
        // Already mapped
        return false;
    }
    
    table[table_idx] = entry_create(phys_addr, flags | VMM_PRESENT);
    
    return true;
}

// Unmap page
bool vmm_unmap_page(void* page_dir, void* virt_addr) {
    if (!page_dir || !virt_addr) {
        return false;
    }
    
    page_dir_t dir = (page_dir_t)page_dir;
    uint32_t dir_idx = VMM_PAGE_DIR_INDEX(virt_addr);
    uint32_t table_idx = VMM_PAGE_TABLE_INDEX(virt_addr);
    
    if (dir_idx >= VMM_PAGE_DIR_ENTRIES || table_idx >= VMM_PAGE_TABLE_ENTRIES) {
        return false;
    }
    
    page_entry_t dir_entry = dir[dir_idx];
    if (!entry_is_present(dir_entry)) {
        return false;
    }
    
    page_table_t table = (page_table_t)entry_get_addr(dir_entry);
    if (!entry_is_present(table[table_idx])) {
        return false;
    }
    
    // Remove page table entry
    table[table_idx] = 0;
    
    return true;
}

// Get physical address from virtual address
void* vmm_get_phys_addr(void* page_dir, void* virt_addr) {
    if (!page_dir || !virt_addr) {
        return NULL;
    }
    
    page_dir_t dir = (page_dir_t)page_dir;
    uint32_t dir_idx = VMM_PAGE_DIR_INDEX(virt_addr);
    uint32_t table_idx = VMM_PAGE_TABLE_INDEX(virt_addr);
    
    if (dir_idx >= VMM_PAGE_DIR_ENTRIES || table_idx >= VMM_PAGE_TABLE_ENTRIES) {
        return NULL;
    }
    
    page_entry_t dir_entry = dir[dir_idx];
    if (!entry_is_present(dir_entry)) {
        return NULL;
    }
    
    page_table_t table = (page_table_t)entry_get_addr(dir_entry);
    if (!entry_is_present(table[table_idx])) {
        return NULL;
    }
    
    void* phys_addr = entry_get_addr(table[table_idx]);
    uint32_t offset = VMM_PAGE_OFFSET(virt_addr);
    
    return (void*)((uint32_t)phys_addr + offset);
}

// Activate page directory
// Note: page_dir must be physical address (before paging enabled, virtual = physical)
void vmm_switch_page_dir(void* page_dir) {
    if (!page_dir) {
        return;
    }
    
    // Verify 4KB alignment
    if ((uint32_t)page_dir & 0xFFF) {
        console_puts("[VMM] ERROR: Page directory not 4KB aligned!\n");
        return;
    }
    
    current_page_dir = page_dir;
    // vmm_flush sets physical address in CR3
    vmm_flush(page_dir);
}

// Get current page directory
void* vmm_get_current_page_dir(void) {
    return current_page_dir;
}

// VMM initialization
void vmm_init(void) {
    console_puts("[VMM] Initializing Virtual Memory Manager...\n");
    
    // Create page directory (returns physical address, before paging enabled virtual = physical)
    void* page_dir = vmm_create_page_dir();
    if (!page_dir) {
        console_puts("[VMM] Failed to create initial page directory\n");
        return;
    }
    
    // Verify 4KB alignment
    if ((uint32_t)page_dir & 0xFFF) {
        console_puts("[VMM] ERROR: Page directory not 4KB aligned!\n");
        return;
    }
    
    // Identity mapping: virtual address = physical address
    // Stack is at 0x900000 (9MB), so need at least 10MB of identity mapping
    // Map 16MB for safety (4096 pages)
    console_puts("[VMM] Creating identity mapping for first 16MB...\n");
    
    uint32_t identity_pages = 4096; // 16MB / 4KB = 4096 pages
    uint32_t mapped_count = 0;
    uint32_t failed_count = 0;
    
    for (uint32_t i = 0; i < identity_pages; i++) {
        void* virt_addr = (void*)(i * VMM_PAGE_SIZE);
        void* phys_addr = virt_addr; // identity mapping
        
        uint32_t flags = VMM_WRITABLE;
        if (i < 256) { // First 1MB is kernel area, user mode access not allowed
            flags &= ~VMM_USER;
        } else {
            flags |= VMM_USER;
        }
        
        if (!vmm_map_page(page_dir, virt_addr, phys_addr, flags)) {
            failed_count++;
        } else {
            mapped_count++;
        }
    }
    
    if (failed_count > 0) {
        console_puts("[VMM] Warning: Failed to map ");
        uint32_t count = failed_count;
        char buf[12];
        int idx = 0;
        if (count == 0) {
            console_putc('0');
        } else {
            while (count > 0 && idx < 11) {
                buf[idx++] = (char)('0' + (count % 10));
                count /= 10;
            }
            while (idx--) console_putc(buf[idx]);
        }
        console_puts(" pages\n");
    }
    
    // Activate page directory
    vmm_switch_page_dir(page_dir);
    
    // Enable paging (set CR0.PG bit)
    // Note: Code executed immediately after paging enable must be within identity mapping range
    uint32_t cr0;
    __asm__ __volatile__("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000; // PG bit (bit 31)
    
    // Enable paging safely using inline assembly
    // Note: Code executed immediately after paging enable must be within identity mapping range
    __asm__ __volatile__(
        "mov %0, %%cr0\n\t"      // Set PG bit in CR0
        "jmp 1f\n\t"             // Jump after paging enable (pipeline flush)
        "1:\n\t"                 // Continue execution here
        "nop\n\t"                // Safety NOP
        :
        : "r"(cr0)
        : "memory", "cc"
    );
    
    console_puts("[VMM] Paging enabled\n");
    console_puts("[VMM] Virtual Memory Manager initialized successfully\n");
}
