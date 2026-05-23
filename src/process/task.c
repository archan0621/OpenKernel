#include "process/task.h"
#include "mem/kmalloc.h"
#include "mem/vmm.h"
#include "drivers/console/console.h"
#include <stddef.h>

// 전역 변수
static uint32_t next_pid = 1;

// 커널 태스크 (idle task)
static task_struct_t kernel_task;

// current_task는 scheduler.c에서 관리
extern task_struct_t* scheduler_get_current_task(void);
extern void scheduler_set_current_task(task_struct_t* task);

static void task_entry_trampoline(void) __attribute__((noreturn));

static void task_entry_trampoline(void) {
    task_struct_t* current = scheduler_get_current_task();

    if (current && current->entry_point) {
        current->entry_point();
    }

    if (current) {
        current->state = TASK_TERMINATED;
        current->time_remaining = 0;
    }

    for (;;) {
        __asm__ __volatile__("sti; hlt");
    }
}

task_struct_t* task_get_kernel_task(void) {
    return &kernel_task;
}

void task_init(void) {
    console_puts("[TASK] Initializing task management...\n");
    
    // 커널 태스크 초기화 (PID 0)
    kernel_task.pid = 0;
    for (int i = 0; i < 32; i++) {
        kernel_task.name[i] = 0;
    }
    kernel_task.name[0] = 'k';
    kernel_task.name[1] = 'e';
    kernel_task.name[2] = 'r';
    kernel_task.name[3] = 'n';
    kernel_task.name[4] = 'e';
    kernel_task.name[5] = 'l';
    
    kernel_task.state = TASK_RUNNING;
    kernel_task.priority = 0;
    kernel_task.time_slice = 10;
    kernel_task.time_remaining = 10;
    kernel_task.next = NULL;
    kernel_task.prev = NULL;
    kernel_task.creation_time = 0;
    kernel_task.cpu_time = 0;
    kernel_task.page_directory = vmm_get_current_page_dir();
    kernel_task.entry_point = NULL;
    
    console_puts("[TASK] Kernel task initialized (PID 0)\n");
}

uint32_t task_get_next_pid(void) {
    return next_pid++;
}

task_struct_t* task_create(const char* name, void (*entry_point)(void), uint32_t priority) {
    // PCB 할당
    task_struct_t* task = (task_struct_t*)kmalloc(sizeof(task_struct_t));
    if (!task) {
        console_puts("[TASK] Failed to allocate task struct\n");
        return NULL;
    }
    
    // 기본 정보 설정
    task->pid = task_get_next_pid();
    
    // 이름 복사
    int i = 0;
    while (name[i] && i < 31) {
        task->name[i] = name[i];
        i++;
    }
    task->name[i] = '\0';
    
    task->state = TASK_READY;
    task->priority = priority;
    task->time_slice = 10;  // 기본 타임 슬라이스
    task->time_remaining = task->time_slice;
    task->creation_time = 0;  // TODO: 타이머 구현 후 실제 시간 설정
    task->cpu_time = 0;
    task->entry_point = entry_point;
    
    // 커널 스택 할당 (4KB) - 태스크별 독립 스택
    task->kernel_stack = (uint32_t)kmalloc(4096);
    if (!task->kernel_stack) {
        console_puts("[TASK] Failed to allocate kernel stack\n");
        kfree(task);
        return NULL;
    }
    
    // 스택은 위에서 아래로 자라므로 스택 포인터는 끝에서 시작
    uint32_t* stack_ptr = (uint32_t*)(task->kernel_stack + 4096);
    
    // ✅ IRQ 프레임 구조에 맞춰 스택 초기화
    // 스택 구조 (낮은 주소 ← 높은 주소):
    // [ds][es][fs][gs][edi][esi][ebp][esp][ebx][edx][ecx][eax][int_no][err_code][eip][cs][eflags]
    //  ↑ top (esp가 가리킴)                                                              ↑ bottom
    
    // IRET이 pop할 값들 (CPU가 자동 처리)
    *(--stack_ptr) = 0x202;                      // EFLAGS (IF 활성화)
    *(--stack_ptr) = 0x08;                       // CS (커널 코드 세그먼트)
    *(--stack_ptr) = (uint32_t)task_entry_trampoline; // EIP
    
    // 인터럽트 번호와 에러 코드
    *(--stack_ptr) = 0;                          // err_code
    *(--stack_ptr) = 32;                         // int_no (IRQ0)
    
    // PUSHA가 push하는 순서: EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    // 즉, 스택 top부터: EDI → ESI → EBP → ESP → EBX → EDX → ECX → EAX
    // POPA는 반대로: EAX부터 pop → ... → EDI
    // 따라서 스택에는 EDI가 맨 위에 있어야 함!
    *(--stack_ptr) = 0;  // eax (맨 아래)
    *(--stack_ptr) = 0;  // ecx
    *(--stack_ptr) = 0;  // edx
    *(--stack_ptr) = 0;  // ebx
    *(--stack_ptr) = 0;  // esp (dummy)
    *(--stack_ptr) = 0;  // ebp
    *(--stack_ptr) = 0;  // esi
    *(--stack_ptr) = 0;  // edi (맨 위, esp가 가리킴)
    
    // 세그먼트 레지스터: push ds, es, fs, gs 순서로 push됨
    // 즉, 스택 top부터: GS → FS → ES → DS
    // pop은 pop gs, fs, es, ds 순서
    // 따라서 스택에는 GS가 맨 위에 있어야 함!
    *(--stack_ptr) = 0x10;  // ds (맨 아래)
    *(--stack_ptr) = 0x10;  // es
    *(--stack_ptr) = 0x10;  // fs
    *(--stack_ptr) = 0x10;  // gs (맨 위, esp가 가리킴)
    
    // esp 저장
    task->esp = stack_ptr;
    
    // 페이지 디렉토리 (현재는 커널 페이지 디렉토리 공유)
    task->page_directory = vmm_get_current_page_dir();
    
    task->next = NULL;
    task->prev = NULL;
    
    console_puts("[TASK] Created task '");
    console_puts(task->name);
    console_puts("' (PID ");
    // PID 출력
    uint32_t pid = task->pid;
    char buf[12];
    int idx = 0;
    if (pid == 0) {
        console_putc('0');
    } else {
        while (pid > 0 && idx < 11) {
            buf[idx++] = (char)('0' + (pid % 10));
            pid /= 10;
        }
        while (idx--) console_putc(buf[idx]);
    }
    console_puts(")\n");
    
    return task;
}

void task_destroy(task_struct_t* task) {
    if (!task) {
        return;
    }
    
    console_puts("[TASK] Destroying task PID ");
    uint32_t pid = task->pid;
    char buf[12];
    int idx = 0;
    if (pid == 0) {
        console_putc('0');
    } else {
        while (pid > 0 && idx < 11) {
            buf[idx++] = (char)('0' + (pid % 10));
            pid /= 10;
        }
        while (idx--) console_putc(buf[idx]);
    }
    console_puts("\n");
    
    // 큐에서 제거
    task_remove_from_ready_queue(task);
    
    // 커널 스택 해제
    if (task->kernel_stack) {
        kfree((void*)task->kernel_stack);
    }
    
    // PCB 해제
    kfree(task);
}

task_struct_t* task_get_current(void) {
    return scheduler_get_current_task();
}

void task_set_current(task_struct_t* task) {
    scheduler_set_current_task(task);
}

void task_set_state(task_struct_t* task, task_state_t state) {
    if (task) {
        task->state = state;
    }
}

task_state_t task_get_state(task_struct_t* task) {
    return task ? task->state : TASK_TERMINATED;
}

// Ready queue 관리는 scheduler.c로 이동
// 이 함수들은 scheduler 함수를 호출하도록 변경
extern void scheduler_add_task(task_struct_t* task);
extern void scheduler_remove_task(task_struct_t* task);

void task_add_to_ready_queue(task_struct_t* task) {
    scheduler_add_task(task);
}

void task_remove_from_ready_queue(task_struct_t* task) {
    scheduler_remove_task(task);
}

task_struct_t* task_get_next_ready(void) {
    // 스케줄러에서 관리하므로 NULL 반환
    return NULL;
}

void task_print_info(task_struct_t* task) {
    if (!task) {
        console_puts("[TASK] NULL task\n");
        return;
    }
    
    console_puts("[TASK] PID: ");
    uint32_t pid = task->pid;
    char buf[12];
    int idx = 0;
    if (pid == 0) {
        console_putc('0');
    } else {
        while (pid > 0 && idx < 11) {
            buf[idx++] = (char)('0' + (pid % 10));
            pid /= 10;
        }
        while (idx--) console_putc(buf[idx]);
    }
    
    console_puts(", Name: ");
    console_puts(task->name);
    
    console_puts(", State: ");
    switch (task->state) {
        case TASK_RUNNING:
            console_puts("RUNNING");
            break;
        case TASK_READY:
            console_puts("READY");
            break;
        case TASK_BLOCKED:
            console_puts("BLOCKED");
            break;
        case TASK_TERMINATED:
            console_puts("TERMINATED");
            break;
    }
    
    console_puts(", Priority: ");
    uint32_t pri = task->priority;
    idx = 0;
    if (pri == 0) {
        console_putc('0');
    } else {
        while (pri > 0 && idx < 11) {
            buf[idx++] = (char)('0' + (pri % 10));
            pri /= 10;
        }
        while (idx--) console_putc(buf[idx]);
    }
    console_puts("\n");
}
