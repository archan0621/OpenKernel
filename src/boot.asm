; src/boot.asm
; Mutli boot header & Kernel main method

[bits 32]
[extern kernel_main]

MULTIBOOT_MAGIC equ 0x1BADB002
MULTIBOOT_FLAGS equ 0x0
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)

section .multiboot
align 4 
  dd MULTIBOOT_MAGIC
  dd MULTIBOOT_FLAGS
  dd MULTIBOOT_CHECKSUM


section .text
global _start
_start:
  ; Start stack from 0x900000
  mov esp, 0x900000

  call kernel_main

.hang:
  cli
  hlt
  jmp .hang 
