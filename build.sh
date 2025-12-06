# /build.sh

#!/bin/bash
set -e

# clean
rm -rf build
mkdir -p build/iso/boot/grub

cp res/grub.cfg build/iso/boot/grub/grub.cfg

# 1) boot.asm to object
nasm -f elf32 src/boot.asm -o build/boot.o

# 2) kernel.c to object
x86_64-elf-gcc -m32 -ffreestanding -O2 -Iinclude -c src/kernel.c -o build/kernel.o

# 3) video.c to object
x86_64-elf-gcc -m32 -ffreestanding -O2 -Iinclude -c video/video.c -o build/video.o

# 4) link 
x86_64-elf-ld -m elf_i386 -T linker.ld -o build/kernel.elf build/boot.o build/kernel.o build/video.o

cp build/kernel.elf build/iso/boot/kernel.bin

i686-elf-grub-mkrescue -o build/archanOS.iso build/iso

echo "✅ build/archanOS.iso 생성 완료"
