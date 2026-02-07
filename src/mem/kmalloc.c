#include "mem/kmalloc.h"
#include "mem/pmm.h"
#include "drivers/console/console.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 블록 헤더 구조체
typedef struct block_header {
    size_t size;                    // 블록 크기 (헤더 제외, 사용자가 사용 가능한 크기)
    bool is_free;                   // 할당 여부
    struct block_header* next;      // 다음 블록
    struct block_header* prev;      // 이전 블록
    uint32_t magic;                 // 매직 넘버 (오버플로우 감지)
} block_header_t;

#define BLOCK_MAGIC 0xDEADBEEF
#define HEADER_SIZE sizeof(block_header_t)
#define ALIGN_SIZE 16
#define MIN_BLOCK_SIZE 32

// 힙 관리 변수
static block_header_t* heap_start = NULL;
static block_header_t* heap_end = NULL;
static uint32_t total_heap_size = 0;  // 전체 힙 크기 (헤더 포함)
static uint32_t used_bytes = 0;       // 할당된 데이터 크기 (헤더 제외)
static uint32_t free_bytes = 0;       // 사용 가능한 데이터 크기 (헤더 제외)

// 크기를 정렬
static inline size_t align_size(size_t size) {
    return (size + ALIGN_SIZE - 1) & ~(ALIGN_SIZE - 1);
}

// 새로운 페이지를 힙에 추가
// pmm_alloc_pages는 원자적으로 동작하므로 부분 할당 없음
static bool expand_heap(size_t min_size) {
    // 필요한 페이지 수 계산
    size_t needed = min_size + HEADER_SIZE;
    uint32_t pages_needed = (needed + PAGE_SIZE - 1) / PAGE_SIZE;
    
    // 연속된 페이지 할당 (원자적: 모두 성공 or 모두 실패)
    // 부분 할당으로 인한 메모리 누수 없음
    void* new_pages = pmm_alloc_pages(pages_needed);
    if (!new_pages) {
        console_puts("[KMALLOC] Failed to allocate ");
        // 간단한 숫자 출력
        uint32_t pn = pages_needed;
        char buf[12];
        int idx = 0;
        while (pn > 0 && idx < 11) {
            buf[idx++] = (char)('0' + (pn % 10));
            pn /= 10;
        }
        while (idx--) console_putc(buf[idx]);
        console_puts(" contiguous pages\n");
        return false;
    }
    
    block_header_t* new_block = (block_header_t*)new_pages;
    size_t block_size = pages_needed * PAGE_SIZE - HEADER_SIZE;
    
    new_block->size = block_size;
    new_block->is_free = true;
    new_block->next = NULL;
    new_block->prev = heap_end;
    new_block->magic = BLOCK_MAGIC;
    
    if (heap_end) {
        heap_end->next = new_block;
    }
    
    heap_end = new_block;
    
    if (!heap_start) {
        heap_start = new_block;
    }
    
    total_heap_size += pages_needed * PAGE_SIZE;
    free_bytes += block_size;
    
    return true;
}

// 블록 분할
// 주의: free_bytes는 호출자가 관리해야 함 (이 함수는 블록 구조만 변경)
static void split_block(block_header_t* block, size_t size) {
    if (block->size < size + HEADER_SIZE + MIN_BLOCK_SIZE) {
        // 분할할 만큼 크지 않음
        return;
    }
    
    size_t old_size = block->size;
    
    // 새 블록 생성 (남는 부분)
    block_header_t* new_block = (block_header_t*)((uint8_t*)block + HEADER_SIZE + size);
    new_block->size = old_size - size - HEADER_SIZE;
    new_block->is_free = true;
    new_block->next = block->next;
    new_block->prev = block;
    new_block->magic = BLOCK_MAGIC;
    
    if (block->next) {
        block->next->prev = new_block;
    } else {
        heap_end = new_block;
    }
    
    block->next = new_block;
    block->size = size;
    
    // 분할 후 free_bytes 조정 필요 없음:
    // - 원래 블록 전체가 free였고
    // - 분할 후에도 (사용 부분 + 헤더 + 남는 free 부분) = 원래 크기
    // - 호출자(kmalloc)가 사용 부분을 free_bytes에서 뺄 것임
}

// 인접한 free 블록 병합
// 앞뒤로 연속된 모든 free 블록을 병합
static void merge_free_blocks(block_header_t* block) {
    if (!block || !block->is_free) {
        return;
    }
    
    // 1단계: 이전 블록들과 병합 (뒤로 이동하면서)
    // block->prev가 free면 block을 prev로 이동
    while (block->prev && block->prev->is_free) {
        block = block->prev;
    }
    
    // 2단계: 현재 위치(가장 앞)에서 다음 블록들과 병합
    while (block->next && block->next->is_free) {
        block_header_t* next_block = block->next;
        
        // 다음 블록을 현재 블록에 병합
        block->size += HEADER_SIZE + next_block->size;
        block->next = next_block->next;
        
        if (block->next) {
            block->next->prev = block;
        } else {
            heap_end = block;
        }
    }
}

void kmalloc_init(void) {
    console_puts("[KMALLOC] Initializing kernel heap allocator...\n");
    
    heap_start = NULL;
    heap_end = NULL;
    total_heap_size = 0;
    used_bytes = 0;
    free_bytes = 0;
    
    // 초기 힙 크기: 4 페이지 (16KB)
    if (!expand_heap(4 * PAGE_SIZE)) {
        console_puts("[KMALLOC] Failed to initialize heap\n");
        return;
    }
    
    console_puts("[KMALLOC] Heap initialized\n");
}

void* kmalloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // 크기 정렬
    size = align_size(size);
    
    // 재귀 대신 루프 사용 (커널에서 재귀는 스택 오버플로우 위험)
    while (1) {
        // First-fit 알고리즘으로 적합한 블록 찾기
        block_header_t* current = heap_start;
        while (current) {
            if (current->is_free && current->size >= size) {
                // 적합한 블록 발견
                size_t old_free_size = current->size;
                
                // 블록 분할 (가능하면)
                split_block(current, size);
                // 분할 후 current->size는 size로 변경됨
                // 남는 부분은 새로운 free 블록으로 생성됨
                
                current->is_free = false;
                
                // free_bytes 업데이트:
                // - 원래 free였던 old_free_size에서
                // - 실제 사용하는 current->size를 뺌
                // - 분할로 생긴 남는 블록은 여전히 free이므로 자동으로 반영됨
                used_bytes += current->size;
                free_bytes -= current->size;
                
                // 데이터 영역 반환 (헤더 다음)
                return (void*)((uint8_t*)current + HEADER_SIZE);
            }
            current = current->next;
        }
        
        // 적합한 블록이 없으면 힙 확장
        if (!expand_heap(size)) {
            console_puts("[KMALLOC] Out of memory\n");
            return NULL;
        }
        
        // 힙 확장 성공, 다시 루프 시작하여 블록 찾기
    }
}

bool kfree(void* ptr) {
    if (!ptr) {
        return false;
    }
    
    // 헤더 위치 계산
    block_header_t* block = (block_header_t*)((uint8_t*)ptr - HEADER_SIZE);
    
    // 매직 넘버 검증
    if (block->magic != BLOCK_MAGIC) {
        console_puts("[KFREE] Invalid magic number - corrupted block\n");
        return false;
    }
    
    // 이미 free된 블록인지 확인 (double free 방지)
    if (block->is_free) {
        console_puts("[KFREE] Double free detected\n");
        return false;
    }
    
    // 블록 해제
    block->is_free = true;
    used_bytes -= block->size;
    free_bytes += block->size;
    
    // 인접 블록 병합
    merge_free_blocks(block);
    
    return true;
}

uint32_t kmalloc_used_bytes(void) {
    return used_bytes;
}

uint32_t kmalloc_free_bytes(void) {
    return free_bytes;
}

uint32_t kmalloc_total_bytes(void) {
    return total_heap_size;
}
