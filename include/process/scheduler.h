#pragma once
#include "process/task.h"
#include <stdint.h>
#include <stdbool.h>

// 스케줄러 초기화
void scheduler_init(void);

// 스케줄링 수행 (다음 태스크로 전환)
void schedule(void);

// 타이머 인터럽트에서 호출 (타임 슬라이스 감소)
void scheduler_tick(void);

// 현재 실행 중인 태스크 가져오기
task_struct_t* scheduler_get_current_task(void);

// 현재 실행 중인 태스크 설정
void scheduler_set_current_task(task_struct_t* task);

// 태스크를 스케줄러에 추가
void scheduler_add_task(task_struct_t* task);

// 태스크를 스케줄러에서 제거
void scheduler_remove_task(task_struct_t* task);

// 스케줄러 통계
uint32_t scheduler_get_total_tasks(void);
void scheduler_print_status(void);

// ✅ IRQ 기반 스케줄러 핸들러
// 인터럽트 프레임 구조체는 scheduler.c에 정의됨
uint32_t scheduler_irq_handler(void* frame);

// 컨텍스트 스위칭 (어셈블리로 구현)
// ESP-only 방식: 스택 포인터만 전달
extern void switch_context(uint32_t** old_esp, uint32_t* new_esp);
