#pragma once

#include <stdint.h>

void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t selector, uint8_t flags);
void idt_enable_interrupts(void);
void idt_disable_interrupts(void);
