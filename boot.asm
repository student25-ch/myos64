; -----------------------------------------------------------------------------
; MyOS Bootloader
; Copyright (C) 2026 student25-ch
;
; This program is free software: you can redistribute it and/or modify
; it under the terms of the GNU General Public License as published by
; the Free Software Foundation, either version 3 of the License, or
; (at your option) any later version.
;
; This program is distributed in the hope that it will be useful,
; but WITHOUT ANY WARRANTY; without even the implied warranty of
; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
; GNU General Public License for more details.
;
; You should have received a copy of the GNU General Public License
; along with this program.  If not, see <https://www.gnu.org/licenses/>.
; -----------------------------------------------------------------------------
[org 0x7c00]
[bits 16]

mov [boot_drive], dl
mov bp, 0x7c00
mov sp, bp

mov ax, 0x03
int 0x10

mov si, boot_msg
call print_str

call check_a20
test ax, ax
jz a20_failure

mov bx, 0x1000
mov es, bx
mov bx, 0x0000
mov ah, 0x02
mov al, 64
mov ch, 0
mov cl, 2
mov dh, 0
mov dl, [boot_drive]
int 0x13
jc disk_fail

cli
lgdt [gdt32_descriptor]
mov eax, cr0
or eax, 1
mov cr0, eax
jmp CODE32_SEG:init_protected

print_str:
    lodsb
    test al, al
    jz .done
    mov ah,0x0e
    int 0x10
    jmp print_str
.done:
ret

check_a20:
    pushf
    push ds
    push es
    mov ax,0x0000
    mov ds,ax
    mov ax,0xffff
    mov es,ax
    mov di,0x0500
    mov si,0x0510
    mov al,[es:di]
    push ax
    mov al,[ds:si]
    push ax
    mov byte [es:di],0x00
    mov byte [ds:si],0xff
    cmp byte [es:di],0xff
    pop ax
    mov [ds:si],al
    pop ax
    mov [es:di],al
    mov ax,0
    je .a20_off
    mov ax,1
.a20_off:
    pop es
    pop ds
    popf
ret

a20_failure:
mov si,err_a20
call print_str
jmp $

disk_fail:
mov si,err_disk
call print_str
jmp $

[bits 32]
init_protected:
    mov esp,0x90000
    mov edi,0x1000
    mov cr3,edi
    xor eax,eax
    mov ecx,4096
    rep stosd

    mov edi,0x1000
    mov dword [edi],0x2003
    add edi,0x1000
    mov dword [edi],0x3003
    add edi,0x1000
    mov dword [edi],0x4003
    add edi,0x1000
    mov dword [edi],0x0083
    mov edi,0x2000
    mov dword [edi],0x0083
    add edi,0x1000
    mov dword [edi],0x0083

    mov eax,cr4
    or eax,1 << 5
    mov cr4,eax

    mov ecx,0xC0000080
    rdmsr
    or eax,1 << 8
    wrmsr

    mov eax,cr0
    or eax,1
    mov cr0,eax

    lgdt [gdt64_descriptor]
    jmp CODE64_SEG:long_mode_entry

[bits 64]
long_mode_entry:
    mov ax,DATA64_SEG
    mov ds,ax
    mov es,ax
    mov fs,ax
    mov gs,ax
    mov ss,ax
    mov rsp,0x90000
    mov rbp,rsp
    extern kernel_main
    call kernel_main
.halt:
    hlt
    jmp .halt

gdt32_start:
gdt32_null: dq 0
gdt32_code:
dw 0xffff
dw 0
db 0
db 0b10011010
db 0b11001111
db 0
gdt32_data:
dw 0xffff
dw 0
db 0
db 0b10010010
db 0b11001111
db 0
gdt32_end:
gdt32_descriptor:
dw gdt32_end - gdt32_start -1
dd gdt32_start

CODE32_SEG equ gdt32_code - gdt32_start
DATA32_SEG equ gdt32_data - gdt32_start

gdt64_start:
gdt64_null: dq 0
gdt64_code: dq 0x0020980000000000
gdt64_data: dq 0x0000900000000000
gdt64_end:
gdt64_descriptor:
dw gdt64_end - gdt64_start -1
dq gdt64_start

CODE64_SEG equ gdt64_code - gdt64_start
DATA64_SEG equ gdt64_data - gdt64_start

boot_msg db "MyOS Bootloader - x86-64 C Kernel | GPLv3",0
err_a20 db "ERROR: A20 Gate failed!",0
err_disk db "ERROR: Disk read failed!",0
boot_drive db 0

times 510 - ($-$$) db 0
dw 0xaa55
