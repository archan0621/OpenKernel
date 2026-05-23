#include "process/scheduler.h"
#include "process/task.h"
#include "drivers/console/console.h"
#include <stddef.h>

// Interrupt frame structure (from irq.asm)
// Stack layout after pusha and segment registers:
struct interrupt_frame {
    uint32_t gs, fs, es, ds;                          // Segment registers
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax;  // pusha order
    uint32_t int_no, err_code;                        // Interrupt number and error code
    uint32_t eip, cs, eflags;                         // CPU pushed
    uint32_t useresp, ss;                             // User mode (if applicable)
} __attribute__((packed));

// 라운드 로빈 큐
static task_struct_t* ready_queue_head = NULL;
static task_struct_t* ready_queue_tail = NULL;
static task_struct_t* current_task = NULL;
static uint32_t total_tasks = 0;
static uint32_t scheduler_ticks = 0;

static void scheduler_enqueue_task(task_struct_t* task) {
    task->next = NULL;
    task->prev = ready_queue_tail;

    if (ready_queue_tail) {
        ready_queue_tail->next = task;
    } else {
        ready_queue_head = task;
    }

    ready_queue_tail = task;
    total_tasks++;
}

static task_struct_t* scheduler_dequeue_task(void) {
    task_struct_t* task = ready_queue_head;
    if (!task) {
        return NULL;
    }

    ready_queue_head = task->next;
    if (ready_queue_head) {
        ready_queue_head->prev = NULL;
    } else {
        ready_queue_tail = NULL;
    }

    task->next = NULL;
    task->prev = NULL;

    if (total_tasks > 0) {
        total_tasks--;
    }

    return task;
}

void scheduler_init(void) {
    console_puts("[SCHEDULER] Initializing round-robin scheduler...\n");
    
    ready_queue_head = NULL;
    ready_queue_tail = NULL;
    total_tasks = 0;
    scheduler_ticks = 0;
    
    // 현재 태스크는 커널 태스크
    current_task = task_get_kernel_task();
    
    console_puts("[SCHEDULER] Scheduler initialized\n");
}

task_struct_t* scheduler_get_current_task(void) {
    return current_task;
}

void scheduler_set_current_task(task_struct_t* task) {
    current_task = task;
}

void scheduler_add_task(task_struct_t* task) {
    if (!task) {
        return;
    }
    
    // 태스크를 READY 상태로 설정
    task_set_state(task, TASK_READY);
    
    scheduler_enqueue_task(task);
    
    console_puts("[SCHEDULER] Added task '");
    console_puts(task->name);
    console_puts("' to ready queue\n");
}

void scheduler_remove_task(task_struct_t* task) {
    if (!task) {
        return;
    }

    bool found = false;
    task_struct_t* current = ready_queue_head;
    while (current) {
        if (current == task) {
            found = true;
            break;
        }
        current = current->next;
    }

    if (!found) {
        return;
    }
    
    // 큐에서 제거
    if (task->prev) {
        task->prev->next = task->next;
    } else {
        ready_queue_head = task->next;
    }
    
    if (task->next) {
        task->next->prev = task->prev;
    } else {
        ready_queue_tail = task->prev;
    }
    
    task->next = NULL;
    task->prev = NULL;
    
    if (total_tasks > 0) {
        total_tasks--;
    }
    
    console_puts("[SCHEDULER] Removed task '");
    console_puts(task->name);
    console_puts("' from ready queue\n");
}

void scheduler_tick(void) {
    // ⚠️ 더 이상 사용 안 함!
    // 타이머 IRQ에서 직접 scheduler_irq_handler를 호출함
    console_puts("[SCHEDULER] WARNING: scheduler_tick() is deprecated!\n");
}

// ⚠️ 주의: 이 함수는 더 이상 직접 호출하지 않음!
// 태스크 전환은 오직 scheduler_irq_handler를 통해서만 발생
void schedule(void) {
    console_puts("[SCHEDULER] WARNING: schedule() is deprecated! Use IRQ-based scheduling.\n");
}

uint32_t scheduler_get_total_tasks(void) {
    return total_tasks;
}

void scheduler_print_status(void) {
    console_puts("[SCHEDULER] Status:\n");
    console_puts("  Total tasks in ready queue: ");
    
    uint32_t count = total_tasks;
    char buf[12];
    int idx = 0;
    if (count == 0) {
        console_putc('0');
    } else {
        while (count > 0 && idx < 11) {
            buf[idx++] = (char)('0' + (count % 10));
            count /= 10;
        }
        while (idx--) console_putc(buf[idx]);
    }
    console_puts("\n");
    
    console_puts("  Current task: ");
    if (current_task) {
        console_puts(current_task->name);
        console_puts(" (PID ");
        
        uint32_t pid = current_task->pid;
        idx = 0;
        if (pid == 0) {
            console_putc('0');
        } else {
            while (pid > 0 && idx < 11) {
                buf[idx++] = (char)('0' + (pid % 10));
                pid /= 10;
            }
            while (idx--) console_putc(buf[idx]);
        }
        console_puts(")");
    } else {
        console_puts("None");
    }
    console_puts("\n");
    
    console_puts("  Scheduler ticks: ");
    count = scheduler_ticks;
    idx = 0;
    if (count == 0) {
        console_putc('0');
    } else {
        while (count > 0 && idx < 11) {
            buf[idx++] = (char)('0' + (count % 10));
            count /= 10;
        }
        while (idx--) console_putc(buf[idx]);
    }
    console_puts("\n");
    
    // Ready 큐의 모든 태스크 출력
    if (ready_queue_head) {
        console_puts("  Ready queue:\n");
        task_struct_t* task = ready_queue_head;
        while (task) {
            console_puts("    - ");
            console_puts(task->name);
            console_puts(" (PID ");
            
            uint32_t pid = task->pid;
            idx = 0;
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
            
            task = task->next;
        }
    }
}

// ✅ IRQ0 (타이머) 핸들러 - 인터럽트 기반 스케줄링
// 반환값: 새로운 esp (태스크 전환 시) 또는 원래 esp (전환 안함)
uint32_t scheduler_irq_handler(void* frame_ptr) {
    struct interrupt_frame* frame = (struct interrupt_frame*)frame_ptr;
    // EOI 전송 (IRQ0은 master PIC)
    __asm__ __volatile__("outb %%al, $0x20" : : "a"(0x20));
    
    scheduler_ticks++;
    
    if (!current_task) {
        // 스택 포인터는 frame 자체를 가리킴
        return (uint32_t)frame;
    }
    
    // 타임 슬라이스 감소
    if (current_task->time_remaining > 0) {
        current_task->time_remaining--;
    }
    
    // 타임 슬라이스가 끝나면 스케줄링
    if (current_task->time_remaining == 0) {
        task_struct_t* old_task = current_task;
        task_struct_t* new_task = NULL;
        
        // 현재 태스크의 ESP 저장
        // frame은 이미 pusha/push 세그먼트 후의 스택을 가리킴
        old_task->esp = (uint32_t*)frame;
        
        // 현재 태스크가 아직 실행 가능하면 큐의 끝으로 이동
        if (old_task->state == TASK_RUNNING) {
            old_task->state = TASK_READY;
            old_task->time_remaining = old_task->time_slice;
            
            // 커널 태스크가 아니면 큐에 다시 추가
            if (old_task->pid != 0) {
                scheduler_enqueue_task(old_task);
            }
        }
        
        // 다음 태스크 선택
        new_task = scheduler_dequeue_task();
        if (!new_task) {
            // Ready 큐가 비어있으면 커널 태스크
            new_task = task_get_kernel_task();
        }
        
        // 태스크 전환
        if (new_task && new_task != old_task) {
            new_task->state = TASK_RUNNING;
            current_task = new_task;
            
            // 새 태스크의 esp 반환
            return (uint32_t)new_task->esp;
        } else {
            // 전환 안함
            if (old_task->state == TASK_READY) {
                old_task->state = TASK_RUNNING;
            }
            return (uint32_t)frame;
        }
    }
    
    // 타임 슬라이스가 남아있으면 현재 태스크 계속 실행
    return (uint32_t)frame;
}
