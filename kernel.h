/*
 * MyOS Kernel Header
 * Copyright (C) 2026 student25-ch
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef __KERNEL_H
#define __KERNEL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define VGA_BUFFER 0xB8000
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_COLOR 0x0F

#define PAGE_SIZE 4096
#define PMM_MAX_BLOCKS 4096
#define HEAP_START 0x2000000
#define HEAP_SIZE (1024*1024*16)

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;
typedef int64_t  s64;
typedef int32_t  s32;
typedef int16_t  s16;
typedef int8_t   s8;

/* VGA Terminal */
void terminal_initialize(void);
void terminal_putchar(char c);
void terminal_write(const char* s);
void terminal_writehex(u64 num);
void terminal_clear(void);
void terminal_setcolor(u8 fg, u8 bg);
size_t terminal_row;
size_t terminal_column;
u8 terminal_color;
u16* terminal_buffer;

/* Memory Manager PMM+VMM + Kernel Heap */
void pmm_init(void);
void* pmm_alloc(void);
void pmm_free(void* ptr);
void vmm_map(u64 cr3, void* va, void* pa, u64 flags);
void vmm_unmap(u64 cr3, void* va);
void heap_init(void);
void* kmalloc(size_t sz);
void kfree(void* ptr);

/* IDT / IRQ / APIC / Interrupt */
struct idt_entry {
    u16 offset_low;
    u16 selector;
    u8 ist;
    u8 flags;
    u16 offset_mid;
    u32 offset_high;
    u32 reserved;
} __attribute__((packed));
struct idt_ptr {
    u16 limit;
    u64 base;
} __attribute__((packed));
void idt_init(void);
void irq_install_handler(int irq, void (*handler)(void));
void apic_init(void);
void ioapic_redirect_irq(u8 irq, u8 vector);

/* PCI Subsystem */
u32 pci_read(u8 bus, u8 dev, u8 func, u8 reg);
void pci_write(u8 bus, u8 dev, u8 func, u8 reg, u32 val);
void pci_scan_bus(void);

/* PS/2 Keyboard Driver */
void ps2kbd_init(void);
char ps2kbd_read_scancode(void);
extern char last_key;

/* Task / Process Manager + Ring3 User Mode */
typedef struct process {
    u64 pid;
    u64 cr3;
    u64 entry;
    u64 rsp_user;
    u64 rsp_kernel;
    char name[32];
    bool active;
    bool is_user;
} process_t;
process_t proc_list[256];
u64 next_pid;
process_t* proc_spawn(u64 entry, const char* name, bool user_mode);
void proc_switch(process_t* next);
void enter_user_mode(u64 entry, u64 user_stack);

/* Multi Binary Format Compatibility Layer */
enum BIN_FORMAT {
    FMT_UNKNOWN = 0,
    FMT_RAW_BIN,
    FMT_DOS_COM,
    FMT_ELF64,
    FMT_PE64,
    FMT_MACHO64
};
enum BIN_FORMAT detect_format(const u8* data, size_t len);
int load_raw_bin(const u8* data, size_t len, process_t* proc);
int load_dos_com(const u8* data, size_t len, process_t* proc);
int load_elf64(const u8* data, size_t len, process_t* proc);
int load_pe64(const u8* data, size_t len, process_t* proc);
int load_macho64(const u8* data, size_t len, process_t* proc);

/* DEB package parser: ar + tar */
typedef struct ar_hdr {
    char name[16];
    char mtime[12];
    char uid[6];
    char gid[6];
    char mode[8];
    char size[10];
    char magic[2];
} ar_hdr_t;
typedef struct tar_header {
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} tar_hdr_t;
int deb_parse(const u8* deb_data, size_t deb_len, u8** out_elf, size_t* out_elf_len);
size_t oct2size(const char* s);

/* DMG / HFS+ parser skeleton */
int dmg_mount(const u8* dmg_img, size_t img_len);
int hfsplus_read_file(const u8* hfs_data, size_t hfs_len, const char* path, u8** out_data, size_t* out_len);

/* FAT32 FileSystem */
struct fat32_boot_sector {
    u8 jmp[3];
    char oem[8];
    u16 bytes_per_sector;
    u8 sectors_per_cluster;
    u16 reserved_sectors;
    u8 fat_count;
    u16 root_entries_16;
    u16 total_sectors_16;
    u8 media;
    u16 fat_size_16;
    u16 sectors_per_track;
    u16 heads;
    u32 hidden_sectors;
    u32 total_sectors_32;
    u32 fat_size_32;
    u16 flags;
    u16 fs_version;
    u32 root_cluster;
    u16 fsinfo_sector;
    u16 backup_boot;
    u8 reserved[12];
    u8 drive_num;
    u8 reserved1;
    u8 boot_sig;
    u32 volume_id;
    char volume_label[11];
    char fs_type[8];
} __attribute__((packed));
void fat32_init(const u8* bootsec);
int fat32_open_file(const char* filename, u8** data_out, size_t* len_out);

/* Disk IO */
u8 disk_read_sector(u8 drive, u32 lba, u8* buf);

/* String Utils */
void memcpy(void* dst, const void* src, size_t n);
void memset(void* dst, u8 val, size_t n);
size_t strlen(const char* s);
int strcmp(const char* a, const char* b);
bool strstart(const char* str, const char* prefix);
void itoa(char* buf, u64 val, int base);

/* Shell CLI */
void shell_init(void);
void shell_input(char c);
void shell_run(void);
extern char shell_buf[256];
extern int shell_pos;

#endif
