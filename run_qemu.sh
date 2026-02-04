#!/bin/bash
# Run the kernel in QEMU with graphical display
qemu-system-i386 -cdrom build/archanOS.iso -no-reboot -no-shutdown
