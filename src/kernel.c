#include <stdint.h>
#include "drivers/console/console.h"
#include "mem/mmap.h"
#include "mem/pmm.h"
#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"

#define MB2_MAGIC 0x36d76289

static void hlt_loop(void) {
    for(;;) __asm__ __volatile__("hlt");
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

void kernel_main(uint32_t magic, void* mbinfo) {
    if (magic != MB2_MAGIC)
        hlt_loop();

    // Initialize console first (needed for GDT verification output)
    console_init(mbinfo);
    console_clear();
    
    // Initialize GDT (will print "GDT OK" if successful)
    gdt_init();

    // Initialize IDT
    idt_init();

	mmap_dump(magic, mbinfo);
    
    // Initialize PMM
    console_puts("\n[PMM] Initializing...\n");
    pmm_init(magic, mbinfo);
    
    // Test: Allocate and free pages
    console_puts("[PMM] Testing page allocation...\n");
    void* page1 = pmm_alloc_page();
    void* page2 = pmm_alloc_page();
    void* page3 = pmm_alloc_page();
    
    if (page1 && page2 && page3) {
        console_puts("[PMM] Allocated 3 pages successfully\n");
        console_puts("[PMM] Free pages after allocation: ");
        console_putu32(pmm_free_pages());
        console_puts("\n");
        
        pmm_free_page(page1);
        pmm_free_page(page2);
        pmm_free_page(page3);
        
        console_puts("[PMM] Freed 3 pages\n");
        console_puts("[PMM] Free pages after free: ");
        console_putu32(pmm_free_pages());
        console_puts("\n");
    } else {
        console_puts("[PMM] Failed to allocate pages\n");
    }
    
    // Test: Put string
    console_puts("Hello World!\n");
    console_puts("Console test: ");
    console_putc('O');
    console_putc('K');
    console_puts("\n");
    console_puts("Tab test:\tTabbed text\n");
    console_puts("Multiple lines:\n");
    console_puts("Line 1\n");
    console_puts("Line 2\n");
    console_puts("Line 3\n");

    hlt_loop();
}