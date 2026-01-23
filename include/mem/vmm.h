#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// 페이지 크기 (4KB)
#define VMM_PAGE_SIZE 4096

// 페이지 디렉토리/테이블 엔트리 수 (각각 1024개)
#define VMM_PAGE_DIR_ENTRIES 1024
#define VMM_PAGE_TABLE_ENTRIES 1024

// 페이지 디렉토리/테이블 크기 (4KB)
#define VMM_PAGE_DIR_SIZE (VMM_PAGE_DIR_ENTRIES * 4)
#define VMM_PAGE_TABLE_SIZE (VMM_PAGE_TABLE_ENTRIES * 4)

// 페이지 엔트리 플래그
#define VMM_PRESENT     (1 << 0)  // 페이지가 메모리에 존재함
#define VMM_WRITABLE    (1 << 1)  // 쓰기 가능
#define VMM_USER        (1 << 2)  // 사용자 모드 접근 가능
#define VMM_PWT         (1 << 3)  // Page Write Through
#define VMM_PCD         (1 << 4)  // Page Cache Disable
#define VMM_ACCESSED    (1 << 5)  // 접근됨
#define VMM_DIRTY       (1 << 6)  // 수정됨 (PTE만)
#define VMM_PAGE_SIZE_4MB (1 << 7) // 4MB 페이지 (PDE만)

// 가상 주소를 페이지 디렉토리/테이블 인덱스로 분해
#define VMM_PAGE_DIR_INDEX(addr)  (((uint32_t)(addr)) >> 22)
#define VMM_PAGE_TABLE_INDEX(addr) ((((uint32_t)(addr)) >> 12) & 0x3FF)
#define VMM_PAGE_OFFSET(addr)     (((uint32_t)(addr)) & 0xFFF)

// VMM 초기화
void vmm_init(void);

// 페이지 디렉토리/테이블 생성 및 관리
void* vmm_create_page_dir(void);
void vmm_destroy_page_dir(void* page_dir);

// 페이지 매핑/언매핑
bool vmm_map_page(void* page_dir, void* virt_addr, void* phys_addr, uint32_t flags);
bool vmm_unmap_page(void* page_dir, void* virt_addr);
void* vmm_get_phys_addr(void* page_dir, void* virt_addr);

// 페이지 디렉토리 활성화 (CR3 레지스터 설정)
void vmm_switch_page_dir(void* page_dir);

// 현재 활성화된 페이지 디렉토리 가져오기
void* vmm_get_current_page_dir(void);

// 페이지 디렉토리/테이블 할당 (PMM 사용)
void* vmm_alloc_page_table(void);
void vmm_free_page_table(void* page_table);
