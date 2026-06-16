/* /linker.ld */
ENTRY(_start)

PHDRS {
  text PT_LOAD FLAGS(5);
  rodata PT_LOAD FLAGS(4);
  data PT_LOAD FLAGS(6);
}

SECTIONS {
  . = 1M;

  kernel_start = .;

  .multiboot : {
    *(.multiboot)
  } :text

  .text : {
    *(.text*)
  } :text

  . = ALIGN(4K);

  .rodata : {
    *(.rodata*)
  } :rodata

  . = ALIGN(4K);

  .data : {
    *(.data*)
  } :data

  .bss : {
    *(COMMON)
    *(.bss*)
  } :data

  kernel_end = .;

}
