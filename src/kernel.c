#include <stdint.h>
#include "drivers/console/console.h"
#include "mem/mmap.h"
#include "mem/pmm.h"
#include "mem/vmm.h"
#include "mem/kmalloc.h"
#include "process/task.h"
#include "process/scheduler.h"
#include "arch/x86/gdt.h"
#include "arch/x86/idt.h"

#define MB2_MAGIC 0x36d76289

static void hlt_loop(void) {
    for(;;) __asm__ __volatile__("hlt");
}

static void console_putu32(uint32_t v) {
    char buf[11];
    int idx = 0;

    if (v == 0) {
        console_putc('0');
        return;
    }

    while (v > 0 && idx < 10) {
        buf[idx++] = (char)('0' + (v % 10));
        v /= 10;
    }

    while (idx--) {
        console_putc(buf[idx]);
    }
}

void kernel_main(uint32_t magic, void* mbinfo) {
    if (magic != MB2_MAGIC)
        hlt_loop();

    // Initialize console first (needed for GDT verification output)
    console_init(mbinfo);
    console_clear();
    
    // Initialize GDT (will print "GDT OK" if successful)
    gdt_init();

    // Initialize IDT
    idt_init();

	mmap_dump(magic, mbinfo);
    
    // Initialize PMM
    console_puts("\n[PMM] Initializing...\n");
    pmm_init(magic, mbinfo);
    
    // Test: Allocate and free pages
    console_puts("[PMM] Testing page allocation...\n");
    void* page1 = pmm_alloc_page();
    void* page2 = pmm_alloc_page();
    void* page3 = pmm_alloc_page();
    
    if (page1 && page2 && page3) {
        console_puts("[PMM] Allocated 3 pages successfully\n");
        console_puts("[PMM] Free pages after allocation: ");
        console_putu32(pmm_get_free_pages());
        console_puts("\n");
        
        pmm_free_page(page1);
        pmm_free_page(page2);
        pmm_free_page(page3);
        
        console_puts("[PMM] Freed 3 pages\n");
        console_puts("[PMM] Free pages after free: ");
        console_putu32(pmm_get_free_pages());
        console_puts("\n");
    } else {
        console_puts("[PMM] Failed to allocate pages\n");
    }
    
    // Initialize VMM
    console_puts("\n[VMM] Initializing Virtual Memory Manager...\n");
    vmm_init();
    
    // Test: VMM 페이지 매핑
    console_puts("[VMM] Testing page mapping...\n");
    void* test_phys = pmm_alloc_page();
    if (test_phys) {
        void* test_virt = (void*)0xC0000000; // 높은 가상 주소에 매핑
        if (vmm_map_page(vmm_get_current_page_dir(), test_virt, test_phys, VMM_WRITABLE)) {
            console_puts("[VMM] Successfully mapped virtual address 0xC0000000\n");
            
            // 물리 주소 확인
            void* mapped_phys = vmm_get_phys_addr(vmm_get_current_page_dir(), test_virt);
            if (mapped_phys == test_phys) {
                console_puts("[VMM] Physical address lookup successful\n");
            }
            
            // 언매핑
            if (vmm_unmap_page(vmm_get_current_page_dir(), test_virt)) {
                console_puts("[VMM] Successfully unmapped page\n");
            }
        }
        pmm_free_page(test_phys);
    }
    
    // Initialize KMALLOC
    console_puts("\n[KMALLOC] Initializing kernel heap...\n");
    kmalloc_init();
    
    // Test: kmalloc and kfree
    console_puts("[KMALLOC] Testing memory allocation...\n");
    
    // 작은 할당 테스트
    char* str1 = (char*)kmalloc(32);
    if (str1) {
        console_puts("[KMALLOC] Allocated 32 bytes\n");
        console_puts("[KMALLOC] Used: ");
        console_putu32(kmalloc_used_bytes());
        console_puts(" bytes\n");
        
        // 메모리에 데이터 쓰기
        for (int i = 0; i < 31; i++) {
            str1[i] = 'A' + (i % 26);
        }
        str1[31] = '\0';
        
        console_puts("[KMALLOC] Written data: ");
        console_puts(str1);
        console_puts("\n");
    }
    
    // 중간 크기 할당 테스트
    int* numbers = (int*)kmalloc(10 * sizeof(int));
    if (numbers) {
        console_puts("[KMALLOC] Allocated array of 10 integers\n");
        console_puts("[KMALLOC] Used: ");
        console_putu32(kmalloc_used_bytes());
        console_puts(" bytes\n");
        
        // 배열에 데이터 쓰기
        for (int i = 0; i < 10; i++) {
            numbers[i] = i * 100;
        }
        
        console_puts("[KMALLOC] Array values: ");
        for (int i = 0; i < 10; i++) {
            console_putu32(numbers[i]);
            console_puts(" ");
        }
        console_puts("\n");
    }
    
    // 큰 할당 테스트
    void* big_block = kmalloc(8192);
    if (big_block) {
        console_puts("[KMALLOC] Allocated 8192 bytes\n");
        console_puts("[KMALLOC] Used: ");
        console_putu32(kmalloc_used_bytes());
        console_puts(" bytes\n");
    }
    
    // 해제 테스트
    console_puts("[KMALLOC] Testing kfree...\n");
    if (kfree(str1)) {
        console_puts("[KMALLOC] Freed str1 successfully\n");
    }
    if (kfree(numbers)) {
        console_puts("[KMALLOC] Freed numbers successfully\n");
    }
    if (kfree(big_block)) {
        console_puts("[KMALLOC] Freed big_block successfully\n");
    }
    
    console_puts("[KMALLOC] After free - Used: ");
    console_putu32(kmalloc_used_bytes());
    console_puts(" bytes, Free: ");
    console_putu32(kmalloc_free_bytes());
    console_puts(" bytes\n");
    
    // Double free 테스트
    console_puts("[KMALLOC] Testing double free detection...\n");
    if (!kfree(str1)) {
        console_puts("[KMALLOC] Double free correctly detected\n");
    }
    
    // 병합 테스트: 연속된 블록 할당 후 순서대로 해제
    console_puts("\n[KMALLOC] Testing block merging...\n");
    void* block1 = kmalloc(64);
    void* block2 = kmalloc(64);
    void* block3 = kmalloc(64);
    
    if (block1 && block2 && block3) {
        console_puts("[KMALLOC] Allocated 3 blocks (64 bytes each)\n");
        console_puts("[KMALLOC] Used: ");
        console_putu32(kmalloc_used_bytes());
        console_puts(" bytes\n");
        
        // 중간 블록 먼저 해제
        kfree(block2);
        console_puts("[KMALLOC] Freed middle block\n");
        
        // 첫 번째 블록 해제 (앞과 병합되어야 함)
        kfree(block1);
        console_puts("[KMALLOC] Freed first block (should merge with middle)\n");
        
        // 마지막 블록 해제 (모두 병합되어야 함)
        kfree(block3);
        console_puts("[KMALLOC] Freed last block (should merge all)\n");
        
        console_puts("[KMALLOC] After merging - Free: ");
        console_putu32(kmalloc_free_bytes());
        console_puts(" bytes\n");
    }
    
    // Initialize Task Management
    console_puts("\n[TASK] Initializing task management...\n");
    task_init();
    
    // Initialize Scheduler
    console_puts("\n[SCHEDULER] Initializing scheduler...\n");
    scheduler_init();
    
    // Test: Task 생성 및 스케줄링
    console_puts("[SCHEDULER] Testing round-robin scheduling...\n");
    
    // 더미 함수 (실제로는 실행 안 됨, 테스트용)
    void dummy_task1(void) {
        console_puts("Task 1 running\n");
    }
    
    void dummy_task2(void) {
        console_puts("Task 2 running\n");
    }
    
    void dummy_task3(void) {
        console_puts("Task 3 running\n");
    }
    
    task_struct_t* task1 = task_create("task1", dummy_task1, 1);
    task_struct_t* task2 = task_create("task2", dummy_task2, 1);
    task_struct_t* task3 = task_create("task3", dummy_task3, 1);
    
    if (task1 && task2 && task3) {
        console_puts("[SCHEDULER] Created 3 tasks successfully\n");
        
        // 태스크 정보 출력
        console_puts("\n[SCHEDULER] Task information:\n");
        task_print_info(scheduler_get_current_task());
        task_print_info(task1);
        task_print_info(task2);
        task_print_info(task3);
        
        // 스케줄러에 추가
        console_puts("\n[SCHEDULER] Adding tasks to scheduler...\n");
        scheduler_add_task(task1);
        scheduler_add_task(task2);
        scheduler_add_task(task3);
        
        // 스케줄러 상태 출력
        console_puts("\n");
        scheduler_print_status();
        
        // 스케줄링 시뮬레이션 (타이머 틱)
        console_puts("\n[SCHEDULER] Simulating timer ticks...\n");
        for (int i = 0; i < 5; i++) {
            console_puts("  Tick ");
            console_putu32(i);
            console_puts(": ");
            
            scheduler_tick();
            
            task_struct_t* current = scheduler_get_current_task();
            if (current) {
                console_puts("Current=");
                console_puts(current->name);
                console_puts(", TimeRemaining=");
                console_putu32(current->time_remaining);
            }
            console_puts("\n");
        }
        
        // 수동 스케줄링 테스트
        console_puts("\n[SCHEDULER] Testing manual scheduling...\n");
        schedule();
        console_puts("\n");
        scheduler_print_status();
        
        schedule();
        console_puts("\n");
        scheduler_print_status();
        
        schedule();
        console_puts("\n");
        scheduler_print_status();
        
        // 태스크 정리
        console_puts("\n[SCHEDULER] Cleaning up tasks...\n");
        task_destroy(task1);
        task_destroy(task2);
        task_destroy(task3);
        console_puts("[SCHEDULER] Tasks destroyed\n");
    }
    
    // Test: Put string
    console_puts("\nHello World!\n");
    console_puts("Console test: ");
    console_putc('O');
    console_putc('K');
    console_puts("\n");
    console_puts("Tab test:\tTabbed text\n");
    console_puts("Multiple lines:\n");
    console_puts("Line 1\n");
    console_puts("Line 2\n");
    console_puts("Line 3\n");

    hlt_loop();
}