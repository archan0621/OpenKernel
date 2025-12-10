#pragma once

#include <stdint.h>

void console_init(void* mbinfo);
void console_clear(void);
void console_putc(char c);
void console_puts(const char* s);

