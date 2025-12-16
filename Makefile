# Makefile for archanOS

# Tools
NASM = nasm
CC = x86_64-elf-gcc
LD = x86_64-elf-ld
GRUB_MKRESCUE = i686-elf-grub-mkrescue

# Flags
NASMFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -O2 -Iinclude
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

# Object files
BOOT_OBJ = $(BUILD_DIR)/boot.o
KERNEL_OBJ = $(BUILD_DIR)/kernel.o
VIDEO_OBJ = $(BUILD_DIR)/video.o
FONT_OBJ = $(BUILD_DIR)/font8x16.o
CONSOLE_OBJ = $(BUILD_DIR)/console.o
GDT_OBJ = $(BUILD_DIR)/gdt.o
GDT_FLUSH_OBJ = $(BUILD_DIR)/gdt_flush.o

# All object files
OBJS = $(BOOT_OBJ) $(KERNEL_OBJ) $(VIDEO_OBJ) $(FONT_OBJ) $(CONSOLE_OBJ) $(GDT_OBJ) $(GDT_FLUSH_OBJ)

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
	@echo "✅ $(ISO) 생성 완료"

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

# Clean build artifacts
clean:
	@echo "Cleaning build directory..."
	rm -rf $(BUILD_DIR)

# Rebuild everything
rebuild: clean all

# Phony targets
.PHONY: all clean rebuild

