#pragma once
#include <stdint.h>
#include <stdbool.h>

// 프로세스 상태
typedef enum {
    TASK_RUNNING,       // 실행 중 또는 실행 가능
    TASK_READY,         // 실행 준비 완료
    TASK_BLOCKED,       // I/O 대기 등으로 블록됨
    TASK_TERMINATED     // 종료됨
} task_state_t;

// 프로세스 제어 블록 (PCB)
// ESP-only 방식: 스택 포인터만 저장
typedef struct task_struct {
    uint32_t pid;                   // 프로세스 ID
    char name[32];                  // 프로세스 이름
    task_state_t state;             // 프로세스 상태
    
    uint32_t* esp;                  // 스택 포인터 (컨텍스트 저장 위치)
    uint32_t kernel_stack;          // 커널 스택 베이스 주소
    uint32_t* page_directory;       // 페이지 디렉토리 (가상 메모리)
    void (*entry_point)(void);      // 태스크 시작 함수
    
    uint32_t priority;              // 우선순위 (0이 가장 높음)
    uint32_t time_slice;            // 타임 슬라이스 (틱 수)
    uint32_t time_remaining;        // 남은 타임 슬라이스
    
    struct task_struct* next;       // 다음 태스크 (링크드 리스트)
    struct task_struct* prev;       // 이전 태스크
    
    uint32_t creation_time;         // 생성 시간
    uint32_t cpu_time;              // 누적 CPU 사용 시간
} task_struct_t;

// 태스크 관리 함수
void task_init(void);
task_struct_t* task_create(const char* name, void (*entry_point)(void), uint32_t priority);
void task_destroy(task_struct_t* task);
void task_exit(void) __attribute__((noreturn));
task_struct_t* task_get_current(void);
void task_set_current(task_struct_t* task);
uint32_t task_get_next_pid(void);
task_struct_t* task_get_kernel_task(void);

// 태스크 상태 관리
void task_set_state(task_struct_t* task, task_state_t state);
task_state_t task_get_state(task_struct_t* task);

// 태스크 리스트 관리
void task_add_to_ready_queue(task_struct_t* task);
void task_remove_from_ready_queue(task_struct_t* task);
task_struct_t* task_get_next_ready(void);

// 디버깅
void task_print_info(task_struct_t* task);
