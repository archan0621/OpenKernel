#include "arch/x86/gdt.h"
#include "drivers/console/console.h"

// gdt entry 8byte
struct gdt_entry {
    uint16_t limit_low; // limit[0:15]
    uint16_t base_low; // base[0:15]
    uint8_t base_middle; // base [16:23]
    uint8_t access; // access flag
    uint8_t granularity; // limit [16:19] etc flag
    uint8_t base_high; // base [24:31]
} __attribute__((packed)); // do not add padding in struct

// gdtr pointer sturct
struct gdt_ptr {
    uint16_t limit; // gdt size
    uint32_t base; // gdt arr base address
} __attribute__((packed)); // do not add padding in struct

static struct gdt_entry gdt[3];
static struct gdt_ptr gp;

// Implemented in assembly (see gdt_flush.asm)
extern void gdt_flush(uint32_t);

// gdt_entry constructor
void gdt_set_gate(
    int num,
    uint32_t base,
    uint32_t limit,
    uint8_t access,
    uint8_t granularity
) {
    gdt[num].base_low    = (base & 0xFFFF);
    gdt[num].base_middle = (base >> 16) & 0xFF;
    gdt[num].base_high   = (base >> 24) & 0xFF;

    gdt[num].limit_low   = (limit & 0xFFFF);
    gdt[num].granularity = (limit >> 16) & 0x0F;

    gdt[num].granularity |= (granularity & 0xF0);
    gdt[num].access      = access;
}

void gdt_init(void) {
    // gdt pointer set
    gp.limit = sizeof(gdt) - 1;
    gp.base  = (uint32_t)&gdt;

    // 0 entry, null descriptor
    gdt_set_gate(0, 0, 0, 0, 0);

    // 1 entry: code segment
    // base = 0x00000000, limit = 0xFFFFF (4GB, 4KB)
    // access = 0x9A -> present, ring0, code, executable, readable
    // gran   = 0xCF -> 4KB granularity, 32bit, limit include top 4 bits
    gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0xCF);

    // 2 entry : data segment
    // access = 0x92 -> present, ring0, data, writable
    gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0xCF);

    // flush gdt info to cpu
    gdt_flush((uint32_t)&gp);
    
    // Verify GDT base is not zero
    struct gdt_ptr_dump {
        uint16_t limit;
        uint32_t base;
    } __attribute__((packed));
    
    struct gdt_ptr_dump gdtr;
    __asm__ __volatile__("sgdt %0" : "=m"(gdtr));
    
    if (gdtr.base != 0) {
        console_puts("GDT OK\n");
    }
}