# MyOS
A custom x86-64 operating system written in Assembly & C
Copyright (C) 2026 student25-ch
Licensed under GPLv3.

## Features
✅ x86_64 Long Mode Bootloader
✅ VGA Text Terminal / CLI Shell
✅ Physical & Virtual Memory Manager + Kernel Heap
✅ PS/2 Keyboard Driver
✅ PCI Bus Scanner
✅ FAT32 File System
✅ Ring3 User Mode Support
✅ Multi binary compatibility layer:
- RAW flat .BIN
- DOS .COM
- ELF64
- PE64 (Windows .EXE)
- Mach-O64 (Apple DMG)
✅ DEB package parser (ar + tar)
✅ DMG / HFS+ mount skeleton

## Build (Windows)
Requirements:
- NASM
- x86_64-elf-gcc (cross compiler)
- x86_64-elf-ld
- QEMU

```bat
build.bat
