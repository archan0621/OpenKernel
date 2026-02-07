#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// 커널 힙 할당자 초기화
void kmalloc_init(void);

// 메모리 할당 (size 바이트)
void* kmalloc(size_t size);

// 메모리 해제 (성공: true, 실패: false)
bool kfree(void* ptr);

// 통계 정보 (모두 헤더 제외한 데이터 크기)
uint32_t kmalloc_used_bytes(void);   // 할당된 데이터 크기
uint32_t kmalloc_free_bytes(void);   // 사용 가능한 데이터 크기
uint32_t kmalloc_total_bytes(void);  // 전체 힙 크기 (헤더 포함)
