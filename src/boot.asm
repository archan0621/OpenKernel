; src/boot.asm
; Multiboot2 header & kernel entry

[bits 32]
[extern kernel_main]

MULTIBOOT2_MAGIC         equ 0xE85250D6
MULTIBOOT2_ARCH          equ 0
MULTIBOOT2_HEADER_LENGTH equ multiboot2_header_end - multiboot2_header_start
MULTIBOOT2_CHECKSUM      equ -(MULTIBOOT2_MAGIC + MULTIBOOT2_ARCH + MULTIBOOT2_HEADER_LENGTH)

section .multiboot
align 8
multiboot2_header_start:
    dd MULTIBOOT2_MAGIC
    dd MULTIBOOT2_ARCH
    dd MULTIBOOT2_HEADER_LENGTH
    dd MULTIBOOT2_CHECKSUM

    dw 0          ; type
    dw 0          ; flags
    dd 8          ; size
multiboot2_header_end:

section .text
global _start
_start:
    ; 스택 설정
    mov esp, 0x900000

    push ebx        ; mb_info
    push eax        ; magic

    call kernel_main

.hang:
    cli
    hlt
    jmp .hang