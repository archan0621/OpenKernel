/* /linker.ld */
ENTRY(_start)

SECTIONS {
  . = 1M;

  kernel_start = .;

  .multiboot : {
    *(.multiboot)
  }

  .text : {
    *(.text*)
  }

  .rodata : {
    *(.rodata*)
  }

  .data : {
    *(.data*)
  }

  .bss : {
    *(COMMON)
    *(.bss*)
  }

  kernel_end = .;

}
