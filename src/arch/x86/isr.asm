[bits 32]

; Common interrupt handler stub
; This pushes the interrupt number and jumps to the C handler
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    cli
    push dword 0         ; dummy error code
    push dword %1        ; interrupt number
    jmp isr_common_stub
%endmacro

%macro ISR_ERRCODE 1
global isr%1
isr%1:
    cli
    push dword %1        ; interrupt number (error code already on stack)
    jmp isr_common_stub
%endmacro

; Exception handlers (ISR 0-31)
ISR_NOERRCODE 0   ; Division by zero
ISR_NOERRCODE 1   ; Debug
ISR_NOERRCODE 2   ; Non-maskable interrupt
ISR_NOERRCODE 3   ; Breakpoint
ISR_NOERRCODE 4   ; Overflow
ISR_NOERRCODE 5   ; Bound range exceeded
ISR_NOERRCODE 6   ; Invalid opcode
ISR_NOERRCODE 7   ; Device not available
ISR_ERRCODE   8   ; Double fault (has error code)
ISR_NOERRCODE 9   ; Coprocessor segment overrun
ISR_ERRCODE   10  ; Invalid TSS (has error code)
ISR_ERRCODE   11  ; Segment not present (has error code)
ISR_ERRCODE   12  ; Stack fault (has error code)
ISR_ERRCODE   13  ; General protection fault (has error code)
ISR_ERRCODE   14  ; Page fault (has error code)
ISR_NOERRCODE 15  ; Reserved
ISR_NOERRCODE 16  ; x87 floating-point exception
ISR_ERRCODE   17  ; Alignment check (has error code)
ISR_NOERRCODE 18  ; Machine check
ISR_NOERRCODE 19  ; SIMD floating-point exception
ISR_NOERRCODE 20  ; Virtualization exception
ISR_NOERRCODE 21  ; Reserved
ISR_NOERRCODE 22  ; Reserved
ISR_NOERRCODE 23  ; Reserved
ISR_NOERRCODE 24  ; Reserved
ISR_NOERRCODE 25  ; Reserved
ISR_NOERRCODE 26  ; Reserved
ISR_NOERRCODE 27  ; Reserved
ISR_NOERRCODE 28  ; Reserved
ISR_NOERRCODE 29  ; Reserved
ISR_NOERRCODE 30  ; Security exception
ISR_NOERRCODE 31  ; Reserved

; External C handler
extern idt_handler

; Common ISR stub
isr_common_stub:
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
    call idt_handler
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

