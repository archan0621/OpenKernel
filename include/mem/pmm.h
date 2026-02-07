#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define PAGE_SIZE 4096u

void pmm_init(uint32_t magic, void* mbinfo);
void* pmm_alloc_page(void);
bool pmm_free_page(void* page);

// 연속된 페이지 할당/해제
void* pmm_alloc_pages(uint32_t count);
bool pmm_free_pages_range(void* page, uint32_t count);

uint32_t pmm_total_pages(void);
uint32_t pmm_get_free_pages(void);

