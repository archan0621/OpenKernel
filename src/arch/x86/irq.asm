[bits 32]

; External C handlers
extern irq_handler
extern scheduler_irq_handler

; IRQ0 (Timer) - Special handler for scheduler
global irq0
irq0:
    cli
    push dword 0     ; dummy error code
    push dword 32    ; interrupt number
    
    ; Save all registers
    pusha
    
    ; Save segment registers
    push ds
    push es
    push fs
    push gs
    
    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Call scheduler handler
    ; ✅ 핵심: esp를 전달하면 scheduler가 esp를 교체할 수 있음!
    push esp                        ; pass pointer to stack frame
    call scheduler_irq_handler      ; 스케줄러 호출
    mov esp, eax                    ; 새 esp 적용! (반환값)
    
    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds
    
    ; Restore all registers
    popa
    
    ; Clean up error code and interrupt number
    add esp, 8
    
    ; Return from interrupt
    iret

; IRQ handler stubs (IRQ 1-15)
%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0     ; dummy error code
    push dword %2    ; interrupt number (IRQ 0-15 map to ISR 32-47)
    jmp irq_common_stub
%endmacro

IRQ 1,  33  ; Keyboard
IRQ 2,  34  ; Cascade
IRQ 3,  35  ; COM2
IRQ 4,  36  ; COM1
IRQ 5,  37  ; LPT2
IRQ 6,  38  ; Floppy
IRQ 7,  39  ; LPT1
IRQ 8,  40  ; CMOS real-time clock
IRQ 9,  41  ; Free
IRQ 10, 42  ; Free
IRQ 11, 43  ; Free
IRQ 12, 44  ; PS/2 mouse
IRQ 13, 45  ; FPU
IRQ 14, 46  ; Primary ATA
IRQ 15, 47  ; Secondary ATA

; Common IRQ stub (for non-timer IRQs)
irq_common_stub:
    ; Save all registers
    pusha

    ; Save segment registers
    push ds
    push es
    push fs
    push gs

    ; Load kernel data segment
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Call C handler
    push esp    ; pass pointer to stack frame
    call irq_handler
    add esp, 4  ; clean up stack

    ; Restore segment registers
    pop gs
    pop fs
    pop es
    pop ds

    ; Restore all registers
    popa

    ; Clean up error code and interrupt number
    add esp, 8

    ; Return from interrupt
    iret

