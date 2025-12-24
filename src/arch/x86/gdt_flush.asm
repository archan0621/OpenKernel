[bits 32]
global gdt_flush

; void gdt_flush(uint32_t gdt_ptr_addr);
gdt_flush:
    mov eax, [esp + 4]   ; param gdt_ptr address

    lgdt [eax]           ; load into GDTR

    ; segment selector : index 1(code) -> 1 * 8 = 0x08
    ;                  index 2(data) -> 2 * 8 = 0x10

    mov ax, 0x10         ; data segment selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; code segment should far jump to flush
    jmp 0x08:.flush_done

.flush_done:
    ret