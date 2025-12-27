#pragma once
#include <stdint.h>
#include <stddef.h>

#define PAGE_SIZE 4096u

void pmm_init(uint32_t magic, void* mbinfo);
void* pmm_alloc_page(void);
void pmm_free_page(void* page);

uint32_t pmm_total_pages(void);
uint32_t pmm_free_pages(void);

