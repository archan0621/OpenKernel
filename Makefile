# Makefile for archanOS

# Tools
NASM = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
GRUB_MKRESCUE = i686-elf-grub-mkrescue
QEMU = qemu-system-i386

# Flags
NASMFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -O2 -Iinclude -mno-sse -mno-sse2 -mno-mmx
LDFLAGS = -m elf_i386 -T linker.ld

# Directories
BUILD_DIR = build
ISO_DIR = $(BUILD_DIR)/iso
GRUB_DIR = $(ISO_DIR)/boot/grub

# Source files
BOOT_SRC = src/boot.asm
KERNEL_SRC = src/kernel.c
VIDEO_SRC = src/drivers/video/video.c
FONT_SRC = src/font/font8x16.c
CONSOLE_SRC = src/drivers/console/console.c
GDT_SRC = src/arch/x86/gdt.c
GDT_FLUSH_SRC = src/arch/x86/gdt_flush.asm
IDT_SRC = src/arch/x86/idt.c
IDT_FLUSH_SRC = src/arch/x86/idt_flush.asm
ISR_SRC = src/arch/x86/isr.asm
IRQ_SRC = src/arch/x86/irq.asm
MMAP_SRC = src/mem/mmap.c
PMM_SRC = src/mem/pmm.c
VMM_SRC = src/mem/vmm.c
VMM_FLUSH_SRC = src/arch/x86/vmm_flush.asm
KMALLOC_SRC = src/mem/kmalloc.c
TASK_SRC = src/process/task.c
SCHEDULER_SRC = src/process/scheduler.c
CONTEXT_SWITCH_SRC = src/arch/x86/context_switch.asm

# Object files
BOOT_OBJ = $(BUILD_DIR)/boot.o
KERNEL_OBJ = $(BUILD_DIR)/kernel.o
VIDEO_OBJ = $(BUILD_DIR)/video.o
FONT_OBJ = $(BUILD_DIR)/font8x16.o
CONSOLE_OBJ = $(BUILD_DIR)/console.o
GDT_OBJ = $(BUILD_DIR)/gdt.o
GDT_FLUSH_OBJ = $(BUILD_DIR)/gdt_flush.o
IDT_OBJ = $(BUILD_DIR)/idt.o
IDT_FLUSH_OBJ = $(BUILD_DIR)/idt_flush.o
ISR_OBJ = $(BUILD_DIR)/isr.o
IRQ_OBJ = $(BUILD_DIR)/irq.o
MMAP_OBJ = $(BUILD_DIR)/mmap.o
PMM_OBJ = $(BUILD_DIR)/pmm.o
VMM_OBJ = $(BUILD_DIR)/vmm.o
VMM_FLUSH_OBJ = $(BUILD_DIR)/vmm_flush.o
KMALLOC_OBJ = $(BUILD_DIR)/kmalloc.o
TASK_OBJ = $(BUILD_DIR)/task.o
SCHEDULER_OBJ = $(BUILD_DIR)/scheduler.o
CONTEXT_SWITCH_OBJ = $(BUILD_DIR)/context_switch.o

# All object files
OBJS = $(BOOT_OBJ) $(KERNEL_OBJ) $(VIDEO_OBJ) $(FONT_OBJ) $(CONSOLE_OBJ) $(GDT_OBJ) $(GDT_FLUSH_OBJ) $(IDT_OBJ) $(IDT_FLUSH_OBJ) $(ISR_OBJ) $(IRQ_OBJ) $(MMAP_OBJ) $(PMM_OBJ) $(VMM_OBJ) $(VMM_FLUSH_OBJ) $(KMALLOC_OBJ) $(TASK_OBJ) $(SCHEDULER_OBJ) $(CONTEXT_SWITCH_OBJ)

# Output files
KERNEL_ELF = $(BUILD_DIR)/kernel.elf
KERNEL_BIN = $(ISO_DIR)/boot/kernel.bin
ISO = $(BUILD_DIR)/archanOS.iso

# Default target
all: $(ISO)

# Create ISO
$(ISO): $(KERNEL_BIN) $(GRUB_DIR)/grub.cfg | $(ISO_DIR)
	@echo "Creating ISO image..."
	$(GRUB_MKRESCUE) -o $(ISO) $(ISO_DIR)
	@echo "✅ ISO image $(ISO) created successfully"

# Copy kernel binary to ISO directory
$(KERNEL_BIN): $(KERNEL_ELF) | $(ISO_DIR)/boot
	cp $(KERNEL_ELF) $(KERNEL_BIN)

# Copy GRUB config
$(GRUB_DIR)/grub.cfg: res/grub.cfg | $(GRUB_DIR)
	cp res/grub.cfg $(GRUB_DIR)/grub.cfg

# Create directories
$(ISO_DIR)/boot:
	@mkdir -p $(ISO_DIR)/boot

$(GRUB_DIR):
	@mkdir -p $(GRUB_DIR)

# Link kernel
$(KERNEL_ELF): $(OBJS) linker.ld
	@echo "Linking kernel..."
	$(LD) $(LDFLAGS) -o $(KERNEL_ELF) $(OBJS)

# Compile boot loader
$(BOOT_OBJ): $(BOOT_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling boot loader..."
	$(NASM) $(NASMFLAGS) $(BOOT_SRC) -o $(BOOT_OBJ)

# Compile kernel
$(KERNEL_OBJ): $(KERNEL_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling kernel..."
	$(CC) $(CFLAGS) -c $(KERNEL_SRC) -o $(KERNEL_OBJ)

# Compile video driver
$(VIDEO_OBJ): $(VIDEO_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling video driver..."
	$(CC) $(CFLAGS) -c $(VIDEO_SRC) -o $(VIDEO_OBJ)

# Compile font
$(FONT_OBJ): $(FONT_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling font..."
	$(CC) $(CFLAGS) -c $(FONT_SRC) -o $(FONT_OBJ)

# Compile console driver
$(CONSOLE_OBJ): $(CONSOLE_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling console driver..."
	$(CC) $(CFLAGS) -c $(CONSOLE_SRC) -o $(CONSOLE_OBJ)

# Compile GDT
$(GDT_OBJ): $(GDT_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling GDT..."
	$(CC) $(CFLAGS) -c $(GDT_SRC) -o $(GDT_OBJ)

# Compile GDT flush (assembly)
$(GDT_FLUSH_OBJ): $(GDT_FLUSH_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling GDT flush..."
	$(NASM) $(NASMFLAGS) $(GDT_FLUSH_SRC) -o $(GDT_FLUSH_OBJ)

# Compile IDT
$(IDT_OBJ): $(IDT_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling IDT..."
	$(CC) $(CFLAGS) -c $(IDT_SRC) -o $(IDT_OBJ)

# Compile IDT flush (assembly)
$(IDT_FLUSH_OBJ): $(IDT_FLUSH_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling IDT flush..."
	$(NASM) $(NASMFLAGS) $(IDT_FLUSH_SRC) -o $(IDT_FLUSH_OBJ)

# Compile ISR (assembly)
$(ISR_OBJ): $(ISR_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling ISR..."
	$(NASM) $(NASMFLAGS) $(ISR_SRC) -o $(ISR_OBJ)

# Compile IRQ (assembly)
$(IRQ_OBJ): $(IRQ_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling IRQ..."
	$(NASM) $(NASMFLAGS) $(IRQ_SRC) -o $(IRQ_OBJ)

# Compile MMAP
$(MMAP_OBJ): $(MMAP_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling MMAP..."
	$(CC) $(CFLAGS) -c $(MMAP_SRC) -o $(MMAP_OBJ)

# Compile PMM
$(PMM_OBJ): $(PMM_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling PMM..."
	$(CC) $(CFLAGS) -c $(PMM_SRC) -o $(PMM_OBJ)

# Compile VMM
$(VMM_OBJ): $(VMM_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling VMM..."
	$(CC) $(CFLAGS) -c $(VMM_SRC) -o $(VMM_OBJ)

# Compile VMM flush (assembly)
$(VMM_FLUSH_OBJ): $(VMM_FLUSH_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling VMM flush..."
	$(NASM) $(NASMFLAGS) $(VMM_FLUSH_SRC) -o $(VMM_FLUSH_OBJ)

# Compile KMALLOC
$(KMALLOC_OBJ): $(KMALLOC_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling KMALLOC..."
	$(CC) $(CFLAGS) -c $(KMALLOC_SRC) -o $(KMALLOC_OBJ)

# Compile TASK
$(TASK_OBJ): $(TASK_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling TASK..."
	$(CC) $(CFLAGS) -c $(TASK_SRC) -o $(TASK_OBJ)

# Compile SCHEDULER
$(SCHEDULER_OBJ): $(SCHEDULER_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling SCHEDULER..."
	$(CC) $(CFLAGS) -c $(SCHEDULER_SRC) -o $(SCHEDULER_OBJ)

# Compile Context Switch (assembly)
$(CONTEXT_SWITCH_OBJ): $(CONTEXT_SWITCH_SRC)
	@mkdir -p $(BUILD_DIR)
	@echo "Compiling Context Switch..."
	$(NASM) $(NASMFLAGS) $(CONTEXT_SWITCH_SRC) -o $(CONTEXT_SWITCH_OBJ)

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

# Rebuild everything
rebuild: clean all

# Run in QEMU
run: $(ISO)
	$(QEMU) -cdrom $(ISO) -no-reboot -no-shutdown

# Phony targets
.PHONY: all clean rebuild run
