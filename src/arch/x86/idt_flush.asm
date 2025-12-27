[bits 32]

; IDT flush function
global idt_flush
idt_flush:
    mov eax, [esp + 4]   ; param idt_ptr address
    lidt [eax]           ; load into IDTR
    ret

