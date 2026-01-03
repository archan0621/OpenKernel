#include "arch/x86/idt.h"
#include "drivers/console/console.h"

// IDT entry structure (8 bytes)
struct idt_entry {
    uint16_t base_low;      // offset bits 0-15
    uint16_t selector;      // code segment selector
    uint8_t  zero;          // reserved, always 0
    uint8_t  flags;         // flags: P, DPL, type
    uint16_t base_high;     // offset bits 16-31
} __attribute__((packed));

// IDTR structure (6 bytes)
struct idt_ptr {
    uint16_t limit;         // IDT size - 1
    uint32_t base;          // IDT base address
} __attribute__((packed));

// IDT with 256 entries (x86 supports 256 interrupts)
static struct idt_entry idt[256];
static struct idt_ptr idtp;

// External assembly function to load IDTR
extern void idt_flush(uint32_t);

// Interrupt handler stubs (will be called from assembly)
extern void isr0();
extern void isr1();
extern void isr2();
extern void isr3();
extern void isr4();
extern void isr5();
extern void isr6();
extern void isr7();
extern void isr8();
extern void isr9();
extern void isr10();
extern void isr11();
extern void isr12();
extern void isr13();
extern void isr14();
extern void isr15();
extern void isr16();
extern void isr17();
extern void isr18();
extern void isr19();
extern void isr20();
extern void isr21();
extern void isr22();
extern void isr23();
extern void isr24();
extern void isr25();
extern void isr26();
extern void isr27();
extern void isr28();
extern void isr29();
extern void isr30();
extern void isr31();

// IRQ handlers
extern void irq0();
extern void irq1();
extern void irq2();
extern void irq3();
extern void irq4();
extern void irq5();
extern void irq6();
extern void irq7();
extern void irq8();
extern void irq9();
extern void irq10();
extern void irq11();
extern void irq12();
extern void irq13();
extern void irq14();
extern void irq15();

// Interrupt stack frame structure
// Stack layout (from top to bottom after pusha and segment registers):
//   GS, FS, ES, DS (pushed manually)
//   EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX (pusha)
//   int_no, err_code (pushed manually)
//   EIP, CS, EFLAGS (pushed by CPU)
//   (useresp, ss only in user mode)
struct interrupt_frame {
    uint32_t gs, fs, es, ds;              // Segment registers (top of stack)
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pusha order
    uint32_t int_no, err_code;            // Interrupt number and error code
    uint32_t eip, cs, eflags;             // CPU pushed registers
    uint32_t useresp, ss;                 // User mode stack (if applicable)
} __attribute__((packed));

// Generic interrupt handler (called from assembly stubs)
void idt_handler(struct interrupt_frame* frame) {
    console_puts("[IDT] Exception occurred: ");
    // Simple number output
    if (frame->int_no < 10) {
        console_putc('0' + frame->int_no);
    } else {
        console_putc('0' + (frame->int_no / 10));
        console_putc('0' + (frame->int_no % 10));
    }
    console_puts("\n");
}

// IRQ handler (called from assembly stubs)
void irq_handler(struct interrupt_frame* frame) {
    // Send EOI to PIC if IRQ >= 8
    if (frame->int_no >= 40) {
        // Send EOI to slave PIC
        __asm__ __volatile__("outb %%al, $0xA0" : : "a"(0x20));
    }
    // Send EOI to master PIC
    __asm__ __volatile__("outb %%al, $0x20" : : "a"(0x20));
    
    // Handle the IRQ
    console_puts("[IDT] IRQ occurred: ");
    uint8_t irq = frame->int_no - 32;
    if (irq < 10) {
        console_putc('0' + irq);
    } else {
        console_putc('0' + (irq / 10));
        console_putc('0' + (irq % 10));
    }
    console_puts("\n");
}

// Set an IDT gate
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags) {
    idt[num].base_low   = base & 0xFFFF;
    idt[num].base_high  = (base >> 16) & 0xFFFF;
    idt[num].selector   = selector;
    idt[num].zero       = 0;
    idt[num].flags      = flags;
}

void idt_init(void) {
    // Set up IDTR
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;

    // Initialize all IDT entries to zero
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0x08, 0);
    }

    // Set up exception handlers (ISR 0-31)
    // Flags: 0x8E = 10001110
    //   Bit 7: Present (1)
    //   Bits 6-5: DPL = 00 (ring 0)
    //   Bit 4: Storage segment = 0 (interrupt gate)
    //   Bits 3-0: Type = 1110 (32-bit interrupt gate)
    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);

    // Set up IRQ handlers (IRQ 0-15 map to ISR 32-47)
    idt_set_gate(32, (uint32_t)irq0,  0x08, 0x8E);
    idt_set_gate(33, (uint32_t)irq1,  0x08, 0x8E);
    idt_set_gate(34, (uint32_t)irq2,  0x08, 0x8E);
    idt_set_gate(35, (uint32_t)irq3,  0x08, 0x8E);
    idt_set_gate(36, (uint32_t)irq4,  0x08, 0x8E);
    idt_set_gate(37, (uint32_t)irq5,  0x08, 0x8E);
    idt_set_gate(38, (uint32_t)irq6,  0x08, 0x8E);
    idt_set_gate(39, (uint32_t)irq7,  0x08, 0x8E);
    idt_set_gate(40, (uint32_t)irq8,  0x08, 0x8E);
    idt_set_gate(41, (uint32_t)irq9,  0x08, 0x8E);
    idt_set_gate(42, (uint32_t)irq10, 0x08, 0x8E);
    idt_set_gate(43, (uint32_t)irq11, 0x08, 0x8E);
    idt_set_gate(44, (uint32_t)irq12, 0x08, 0x8E);
    idt_set_gate(45, (uint32_t)irq13, 0x08, 0x8E);
    idt_set_gate(46, (uint32_t)irq14, 0x08, 0x8E);
    idt_set_gate(47, (uint32_t)irq15, 0x08, 0x8E);

    // Load IDTR
    idt_flush((uint32_t)&idtp);

    console_puts("[IDT] Initialized\n");
}

