#!/bin/bash
set -euo pipefail

ISO="build/archanOS.iso"

if [ ! -f "$ISO" ]; then
    echo "[run_qemu] $ISO not found. Building it with make..."
    make
fi

exec qemu-system-i386 -cdrom "$ISO" -no-reboot -no-shutdown
