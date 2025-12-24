#include <stdint.h>
#include "drivers/console/console.h"
#include "arch/x86/gdt.h"

#define MB2_MAGIC 0x36d76289

static void hlt_loop(void) {
    for(;;) __asm__ __volatile__("hlt");
}

void kernel_main(uint32_t magic, void* mbinfo) {
    if (magic != MB2_MAGIC)
        hlt_loop();

    // Initialize console first (needed for GDT verification output)
    console_init(mbinfo);
    console_clear();
    
    // Initialize GDT (will print "GDT OK" if successful)
    gdt_init();
    
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