/*
 * MyOS Linker Script
 * Copyright (C) 2026 student25-ch
 * GPLv3
 */
ENTRY(long_mode_entry)
SECTIONS
{
    . = 1M;
    .text ALIGN(4K) :
    {
        *(.boot)
        *(.text*)
    }
    .rodata ALIGN(4K) :
    {
        *(.rodata*)
    }
    .data ALIGN(4K) :
    {
        *(.data*)
    }
    .bss ALIGN(4K) :
    {
        *(COMMON)
        *(.bss*)
    }
}
