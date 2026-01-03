[bits 32]

; IRQ handler stubs
%macro IRQ 2
global irq%1
irq%1:
    cli
    push dword 0     ; dummy error code
    push dword %2    ; interrupt number (IRQ 0-15 map to ISR 32-47)
    jmp irq_common_stub
%endmacro

; IRQ handlers (IRQ 0-15 map to ISR 32-47)
IRQ 0,  32  ; Timer
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

; External C handler
extern irq_handler

; Common IRQ stub
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

