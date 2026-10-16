@echo off
REM MyOS Build Script GPLv3
echo Building MyOS x86_64 Full Version (All 7 Modules)
nasm -f elf64 boot.asm -o boot.o
x86_64-elf-gcc -ffreestanding -mno-red-zone -c kernel.c -o kernel.o
x86_64-elf-ld -n -o kernel.bin -T linker.ld boot.o kernel.o
copy /b boot.bin + kernel.bin os-image.bin
echo Build complete.
echo Run command: qemu-system-x86_64 -drive format=raw,file=os-image.bin
pause
