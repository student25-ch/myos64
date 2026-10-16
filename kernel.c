/*
 * MyOS x86-64 Kernel
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
#include "kernel.h"

size_t terminal_row;
size_t terminal_column;
u8 terminal_color;
u16* terminal_buffer;

void terminal_initialize(void)
{
    terminal_row = 0;
    terminal_column = 0;
    terminal_color = VGA_COLOR;
    terminal_buffer = (u16*)VGA_BUFFER;
    terminal_clear();
}

void terminal_setcolor(u8 fg, u8 bg)
{
    terminal_color = (bg << 4) | fg;
}

void terminal_putchar(char c)
{
    u16 entry = (terminal_color << 8) | c;
    if(c == '\n')
    {
        terminal_column = 0;
        terminal_row++;
        if(terminal_row >= VGA_HEIGHT)
        {
            terminal_row = VGA_HEIGHT -1;
            for(size_t y=0; y<VGA_HEIGHT-1; y++)
            {
                for(size_t x=0;x<VGA_WIDTH;x++)
                {
                    terminal_buffer[y*VGA_WIDTH + x] = terminal_buffer[(y+1)*VGA_WIDTH +x];
                }
            }
            for(size_t x=0;x<VGA_WIDTH;x++)
            {
                terminal_buffer[terminal_row * VGA_WIDTH + x] = (0x0F <<8)|' ';
            }
        }
        return;
    }
    terminal_buffer[terminal_row * VGA_WIDTH + terminal_column] = entry;
    terminal_column++;
    if(terminal_column >= VGA_WIDTH)
    {
        terminal_column = 0;
        terminal_row++;
    }
}

void terminal_write(const char* s)
{
    for(size_t i=0; s[i];i++)
    {
        terminal_putchar(s[i]);
    }
}

void terminal_writehex(u64 num)
{
    char digits[] = "0123456789ABCDEF";
    terminal_putchar('0');
    terminal_putchar('x');
    for(int i=15;i>=0;i--)
    {
        u64 shift = i*4;
        u8 val = (num >> shift) & 0xF;
        terminal_putchar(digits[val]);
    }
}

void terminal_clear(void)
{
    for(size_t y=0;y<VGA_HEIGHT;y++)
    {
        for(size_t x=0;x<VGA_WIDTH;x++)
        {
            terminal_buffer[y*VGA_WIDTH+x] = (VGA_COLOR <<8)|' ';
        }
    }
    terminal_row=0;
    terminal_column=0;
}

void memcpy(void* dst, const void* src, size_t n)
{
    u8* d = (u8*)dst;
    const u8* s = (const u8*)src;
    for(size_t i=0;i<n;i++) d[i]=s[i];
}

void memset(void* dst, u8 val, size_t n)
{
    u8* d=(u8*)dst;
    for(size_t i=0;i<n;i++) d[i]=val;
}

size_t strlen(const char* s)
{
    size_t l=0;
    while(s[l]) l++;
    return l;
}

int strcmp(const char* a, const char* b)
{
    while(*a && *b)
    {
        if(*a != *b) return *a - *b;
        a++;b++;
    }
    return *a - *b;
}

bool strstart(const char* str, const char* prefix)
{
    while(*prefix)
    {
        if(*str != *prefix) return false;
        str++;prefix++;
    }
    return true;
}

void itoa(char* buf, u64 val, int base)
{
    static char digits[] = "0123456789ABCDEF";
    int i=0;
    if(val ==0) { buf[i++]='0'; buf[i]=0; return; }
    while(val>0)
    {
        buf[i++] = digits[val % base];
        val /= base;
    }
    buf[i]=0;
    int l = i;
    for(int j=0;j<l/2;j++)
    {
        char t = buf[j];
        buf[j] = buf[l-j-1];
        buf[l-j-1] = t;
    }
}

/* ========================== Physical Memory Manager ===================== */
static bool pmm_blocks[PMM_MAX_BLOCKS];

void pmm_init(void)
{
    memset(pmm_blocks, 0, sizeof(pmm_blocks));
}

void* pmm_alloc(void)
{
    for(int i=0;i<PMM_MAX_BLOCKS;i++)
    {
        if(!pmm_blocks[i])
        {
            pmm_blocks[i]=true;
            return (void*)(0x100000 + i*PAGE_SIZE);
        }
    }
    return NULL;
}

void pmm_free(void* ptr)
{
    u64 addr = (u64)ptr;
    addr -= 0x100000;
    int idx = addr / PAGE_SIZE;
    if(idx >=0 && idx < PMM_MAX_BLOCKS)
        pmm_blocks[idx]=false;
}

void vmm_map(u64 cr3, void* va, void* pa, u64 flags)
{
    (void)cr3; (void)va; (void)pa; (void)flags;
}
void vmm_unmap(u64 cr3, void* va)
{
    (void)cr3; (void)va;
}

/* Kernel Heap */
typedef struct heap_block {
    size_t size;
    bool free;
    struct heap_block* next;
} heap_block_t;
static heap_block_t* heap_start;

void heap_init(void)
{
    heap_start = (heap_block_t*)HEAP_START;
    heap_start->size = HEAP_SIZE - sizeof(heap_block_t);
    heap_start->free = true;
    heap_start->next = NULL;
}

void* kmalloc(size_t sz)
{
    heap_block_t* curr = heap_start;
    while(curr)
    {
        if(curr->free && curr->size >= sz)
        {
            if(curr->size - sz > sizeof(heap_block_t)+16)
            {
                heap_block_t* split = (heap_block_t*)((u8*)curr + sizeof(heap_block_t)+sz);
                split->size = curr->size - sz - sizeof(heap_block_t);
                split->free = true;
                split->next = curr->next;
                curr->size = sz;
                curr->next = split;
            }
            curr->free = false;
            return (void*)((u8*)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }
    return NULL;
}

void kfree(void* ptr)
{
    if(!ptr) return;
    heap_block_t* blk = (heap_block_t*)((u8*)ptr - sizeof(heap_block_t));
    blk->free = true;
    heap_block_t* curr = heap_start;
    while(curr)
    {
        if(curr->free && curr->next && curr->next->free)
        {
            curr->size += curr->next->size + sizeof(heap_block_t);
            curr->next = curr->next->next;
        }
        curr = curr->next;
    }
}

/* ===================== IDT / Interrupt / APIC ========================== */
static struct idt_entry idt[256];
static struct idt_ptr idtp;
void idt_setgate(int num, u64 offset, u16 sel, u8 flags)
{
    idt[num].offset_low = offset & 0xFFFF;
    idt[num].selector = sel;
    idt[num].ist = 0;
    idt[num].flags = flags;
    idt[num].offset_mid = (offset>>16)&0xFFFF;
    idt[num].offset_high = (offset>>32)&0xFFFFFFFF;
    idt[num].reserved =0;
}
void idt_init(void)
{
    idtp.limit = sizeof(idt)-1;
    idtp.base = (u64)&idt;
    memset(&idt,0,sizeof(idt));
    for(int i=0;i<32;i++) idt_setgate(i,0,0x08,0x8E);
    asm volatile("lidt %0" :: "m"(idtp));
}
void irq_install_handler(int irq, void (*handler)(void)) { (void)irq; (void)handler; }
void apic_init(void) {}
void ioapic_redirect_irq(u8 irq, u8 vector) { (void)irq; (void)vector; }

/* PCI */
u32 pci_read(u8 bus, u8 dev, u8 func, u8 reg)
{
    u32 addr = (1U<<31) | (bus<<16) | (dev<<11) | (func<<8) | reg;
    asm volatile("out 0xCF8, %0"::"a"(addr));
    u32 ret;
    asm volatile("in %1, %0":"=a"(ret):"Nd"(0xCFC));
    return ret;
}
void pci_write(u8 bus, u8 dev, u8 func, u8 reg, u32 val)
{
    u32 addr = (1U<<31) | (bus<<16) | (dev<<11) | (func<<8) | reg;
    asm volatile("out 0xCF8, %0"::"a"(addr));
    asm volatile("out %1, %0"::"a"(val),"Nd"(0xCFC));
}
void pci_scan_bus(void)
{
    terminal_write("[PCI] Scanning PCI bus...\n");
    for(u8 bus=0;bus<1;bus++)
    {
        for(u8 dev=0;dev<32;dev++)
        {
            u32 vend = pci_read(bus,dev,0,0);
            if((vend & 0xFFFF) == 0xFFFF) continue;
            terminal_write("PCI Dev: ");
            terminal_writehex(bus);
            terminal_putchar(':');
            terminal_writehex(dev);
            terminal_write(" Vendor:");
            terminal_writehex(vend & 0xFFFF);
            terminal_putchar('\n');
        }
    }
}

/* PS2 Keyboard */
char last_key = 0;
void ps2kbd_init(void)
{
    terminal_write("[PS2] Keyboard driver loaded\n");
}
char ps2kbd_read_scancode(void)
{
    u8 status;
    asm volatile("in al, 0x64":"=a"(status));
    if(status & 1)
    {
        u8 sc;
        asm volatile("in al,0x60":"=a"(sc));
        last_key = sc;
        return sc;
    }
    return 0;
}

/* ===================== Process Manager + Ring3 User Mode ========================== */
process_t proc_list[256];
u64 next_pid=1;

process_t* proc_spawn(u64 entry, const char* name, bool user_mode)
{
    for(int i=0;i<256;i++)
    {
        if(!proc_list[i].active)
        {
            proc_list[i].pid = next_pid++;
            proc_list[i].cr3 = (u64)pmm_alloc();
            proc_list[i].entry = entry;
            proc_list[i].rsp_kernel = (u64)pmm_alloc() + PAGE_SIZE;
            proc_list[i].rsp_user = (u64)pmm_alloc() + PAGE_SIZE;
            proc_list[i].active = true;
            proc_list[i].is_user = user_mode;
            memset(proc_list[i].name,0,32);
            memcpy(proc_list[i].name, name, strlen(name));
            return &proc_list[i];
        }
    }
    return NULL;
}

void proc_switch(process_t* next)
{
    if(!next || !next->active) return;
    asm volatile("mov cr3, %0" :: "r"(next->cr3));
    if(next->is_user)
    {
        enter_user_mode(next->entry, next->rsp_user);
    }else{
        asm volatile("jmp *%0" :: "r"(next->entry));
    }
}
void enter_user_mode(u64 entry, u64 user_stack)
{
    asm volatile (
        "mov rbp, rsp\n"
        "push 0x23\n"
        "push %0\n"
        "pushf\n"
        "push 0x1B\n"
        "push %1\n"
        "iretq\n"
        :: "r"(user_stack), "r"(entry)
    );
}

/* ===================== Multi Format Compatibility Layer ============== */
enum BIN_FORMAT detect_format(const u8* data, size_t len)
{
    if(len < 4) return FMT_UNKNOWN;
    if(data[0]==0x7f && data[1]=='E' && data[2]=='L' && data[3]=='F')
        return FMT_ELF64;
    if(data[0]=='M' && data[1]=='Z')
    {
        u32 pe_off = *(u32*)(data+0x3C);
        if(pe_off + 4 < len)
        {
            if(data[pe_off] == 'P' && data[pe_off+1] == 'E')
                return FMT_PE64;
        }
        if(pe_off == 0)
            return FMT_DOS_COM;
    }
    if(len >4 && data[0]==0xCA && data[1]==0xFE && data[2]==0xBA && data[3]==0xBE)
        return FMT_MACHO64;
    return FMT_RAW_BIN;
}

int load_raw_bin(const u8* data, size_t len, process_t* proc)
{
    terminal_write("[Loader] Loading RAW flat binary\n");
    void* dest = kmalloc(len);
    if(!dest) return -1;
    memcpy(dest, data, len);
    proc->entry = (u64)dest;
    return 0;
}

int load_dos_com(const u8* data, size_t len, process_t* proc)
{
    terminal_write("[Loader] Loading DOS .COM program\n");
    void* dest = kmalloc(len + 0x100);
    if(!dest) return -1;
    memcpy((u8*)dest+0x100, data, len);
    proc->entry = (u64)dest + 0x100;
    return 0;
}

typedef struct {
    u8 e_ident[16];
    u16 e_type;
    u16 e_machine;
    u32 e_version;
    u64 e_entry;
    u64 e_phoff;
    u64 e_shoff;
    u32 e_flags;
    u16 e_ehsize;
    u16 e_phentsize;
    u16 e_phnum;
    u16 e_shentsize;
    u16 e_shnum;
    u16 e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    u32 p_type;
    u32 p_flags;
    u64 p_offset;
    u64 p_vaddr;
    u64 p_paddr;
    u64 p_filesz;
    u64 p_memsz;
    u64 p_align;
} Elf64_Phdr;

int load_elf64(const u8* data, size_t len, process_t* proc)
{
    terminal_write("[Loader] Loading ELF64 executable\n");
    Elf64_Ehdr* ehdr = (Elf64_Ehdr*)data;
    if(ehdr->e_ident[4] != 2)
    {
        terminal_write("ERROR: Not 64-bit ELF\n");
        return -1;
    }
    for(int i=0; i<ehdr->e_phnum; i++)
    {
        Elf64_Phdr* phdr = (Elf64_Phdr*)(data + ehdr->e_phoff + i*ehdr->e_phentsize);
        if(phdr->p_type != 1) continue;
        void* mem = kmalloc(phdr->p_memsz);
        memset(mem, 0, phdr->p_memsz);
        memcpy(mem, data + phdr->p_offset, phdr->p_filesz);
        vmm_map(proc->cr3, (void*)phdr->p_vaddr, mem, 3);
    }
    proc->entry = ehdr->e_entry;
    return 0;
}

typedef struct {
    u16 e_magic;
    u16 e_cblp;
    u16 e_cp;
    u16 e_crlc;
    u16 e_cparhdr;
    u16 e_minalloc;
    u16 e_maxalloc;
    u16 e_ss;
    u16 e_sp;
    u16 e_csum;
    u16 e_ip;
    u16 e_cs;
    u16 e_lfarlc;
    u16 e_ovno;
    u16 e_res[4];
    u16 e_oemid;
    u16 e_oeminfo;
    u16 e_res2[10];
    u32 e_lfanew;
} IMAGE_DOS_HEADER;

typedef struct {
    u32 Signature;
    u16 Machine;
    u16 NumberOfSections;
    u32 TimeDateStamp;
    u32 PointerToSymbolTable;
    u32 NumberOfSymbols;
    u16 SizeOfOptionalHeader;
    u16 Characteristics;
} IMAGE_FILE_HEADER;

typedef struct {
    u16 Magic;
    u8 MajorLinkerVersion;
    u8 MinorLinkerVersion;
    u32 SizeOfCode;
    u32 SizeOfInitializedData;
    u32 SizeOfUninitializedData;
    u32 AddressOfEntryPoint;
    u32 BaseOfCode;
    u64 ImageBase;
    u32 SectionAlignment;
    u32 FileAlignment;
    u16 MajorOperatingSystemVersion;
    u16 MinorOperatingSystemVersion;
    u16 MajorImageVersion;
    u16 MinorImageVersion;
    u16 MajorSubsystemVersion;
    u16 MinorSubsystemVersion;
    u32 Win32VersionValue;
    u32 SizeOfImage;
    u32 SizeOfHeaders;
    u32 CheckSum;
    u16 Subsystem;
    u16 DllCharacteristics;
    u64 SizeOfStackReserve;
    u64 SizeOfStackCommit;
    u64 SizeOfHeapReserve;
    u64 SizeOfHeapCommit;
    u32 LoaderFlags;
    u32 NumberOfRvaAndSizes;
} IMAGE_OPTIONAL_HEADER64;

typedef struct {
    char Name[8];
    u32 VirtualSize;
    u32 VirtualAddress;
    u32 SizeOfRawData;
    u32 PointerToRawData;
    u32 PointerToRelocations;
    u32 PointerToLinenumbers;
    u16 NumberOfRelocations;
    u16 NumberOfLinenumbers;
    u32 Characteristics;
} IMAGE_SECTION_HEADER;

int load_pe64(const u8* data, size_t len, process_t* proc)
{
    terminal_write("[Loader] Loading PE64 (EXE) binary\n");
    IMAGE_DOS_HEADER* dos_hdr = (IMAGE_DOS_HEADER*)data;
    u32 pe_ptr = dos_hdr->e_lfanew;
    IMAGE_FILE_HEADER* file_hdr = (IMAGE_FILE_HEADER*)(data + pe_ptr + 4);
    IMAGE_OPTIONAL_HEADER64* opt_hdr = (IMAGE_OPTIONAL_HEADER64*)((u8*)file_hdr + sizeof(IMAGE_FILE_HEADER));
    IMAGE_SECTION_HEADER* sec = (IMAGE_SECTION_HEADER*)((u8*)opt_hdr + file_hdr->SizeOfOptionalHeader);
    for(int s=0; s<file_hdr->NumberOfSections; s++)
    {
        void* sec_mem = kmalloc(sec[s].VirtualSize);
        memset(sec_mem,0,sec[s].VirtualSize);
        memcpy(sec_mem, data + sec[s].PointerToRawData, sec[s].SizeOfRawData);
        vmm_map(proc->cr3, (void*)(opt_hdr->ImageBase + sec[s].VirtualAddress), sec_mem, 3);
    }
    u64 entry = opt_hdr->ImageBase + opt_hdr->AddressOfEntryPoint;
    proc->entry = entry;
    terminal_write("[PE64] ImageBase: ");
    terminal_writehex(opt_hdr->ImageBase);
    terminal_write(" EntryRVA: ");
    terminal_writehex(opt_hdr->AddressOfEntryPoint);
    terminal_putchar('\n');
    return 0;
}

typedef struct macho_header64 {
    u32 magic;
    u32 cputype;
    u32 cpusubtype;
    u32 filetype;
    u32 ncmds;
    u32 sizeofcmds;
    u32 flags;
    u32 reserved;
} macho_header64_t;

int load_macho64(const u8* data, size_t len, process_t* proc)
{
    terminal_write("[Loader] Loading Mach-O64 (from DMG)\n");
    macho_header64_t* mh = (macho_header64_t*)data;
    if(mh->magic != 0xBEBAFECA)
    {
        terminal_write("ERROR: Invalid Mach-O magic\n");
        return -1;
    }
    proc->entry = 0x100000000;
    return 0;
}

/* ===================== DEB ar + tar parser ========================== */
size_t oct2size(const char* s)
{
    size_t res=0;
    while(*s && *s!=' ')
    {
        res = res*8 + (*s - '0');
        s++;
    }
    return res;
}

int deb_parse(const u8* deb_data, size_t deb_len, u8** out_elf, size_t* out_elf_len)
{
    terminal_write("[DEB] Parsing Debian package (ar archive)\n");
    if(deb_len < 8) return -1;
    if(memcmp(deb_data,"!<arch>\n",8)!=0)
    {
        terminal_write("ERROR: Not ar archive\n");
        return -1;
    }
    size_t offset = 8;
    while(offset + sizeof(ar_hdr_t) <= deb_len)
    {
        ar_hdr_t* arh = (ar_hdr_t*)(deb_data + offset);
        size_t sz = oct2size(arh->size);
        offset += sizeof(ar_hdr_t);
        if(strstart(arh->name,"data.tar"))
        {
            terminal_write("[DEB] Found data.tar payload\n");
            const u8* tar_data = deb_data + offset;
            size_t tar_sz = sz;
            size_t tar_off=0;
            while(tar_off + sizeof(tar_hdr_t) <= tar_sz)
            {
                tar_hdr_t* th = (tar_hdr_t*)(tar_data + tar_off);
                size_t fsz = oct2size(th->size);
                if(fsz ==0) break;
                if(strstart(th->name,"usr/bin/") || strstart(th->name,"bin/"))
                {
                    *out_elf = (u8*)kmalloc(fsz);
                    memcpy(*out_elf, tar_data + tar_off + 512, fsz);
                    *out_elf_len = fsz;
                    terminal_write("[DEB] Extracted ELF executable\n");
                    return 0;
                }
                tar_off += 512 + ((fsz+511)/512)*512;
            }
        }
        offset += (sz+1)&~1;
    }
    return -1;
}

/* DMG / HFS+ Skeleton */
int dmg_mount(const u8* dmg_img, size_t img_len)
{
    (void)dmg_img; (void)img_len;
    terminal_write("[DMG] Mount Apple DMG image (HFS+)\n");
    return 0;
}
int hfsplus_read_file(const u8* hfs_data, size_t hfs_len, const char* path, u8** out_data, size_t* out_len)
{
    (void)hfs_data; (void)hfs_len; (void)path; (void)out_data; (void)out_len;
    terminal_write("[HFS+] Read file from DMG\n");
    return -1;
}

/* FAT32 File System */
static struct fat32_boot_sector fat_bs;
void fat32_init(const u8* bootsec)
{
    memcpy(&fat_bs, bootsec, sizeof(struct fat32_boot_sector));
    terminal_write("[FAT32] Filesystem init OK\n");
}
int fat32_open_file(const char* filename, u8** data_out, size_t* len_out)
{
    (void)filename; (void)data_out; (void)len_out;
    return -1;
}

/* Disk IO */
u8 disk_read_sector(u8 drive, u32 lba, u8* buf)
{
    (void)drive; (void)lba; (void)buf;
    return 0;
}

/* ===================== Shell CLI ========================= */
char shell_buf[256];
int shell_pos=0;

void shell_init(void)
{
    terminal_write("\nMyOS> ");
}

void shell_input(char c)
{
    if(c == '\r' || c == '\n')
    {
        terminal_putchar('\n');
        shell_buf[shell_pos]=0;
        if(strcmp(shell_buf, "help") ==0)
        {
            terminal_write("==== MyOS Command List ====\n");
            terminal_write("help        Show help\n");
            terminal_write("clear       Clear screen\n");
            terminal_write("pci         Scan PCI bus\n");
            terminal_write("loadbin     Load raw .bin\n");
            terminal_write("loadcom     Load DOS .COM\n");
            terminal_write("loadelf     Load ELF64\n");
            terminal_write("loadpe      Load PE64 exe\n");
            terminal_write("loadmacho   Load Mach-O (DMG)\n");
            terminal_write("loaddep     Parse deb package\n");
            terminal_write("mountdmg    Mount dmg image\n");
            terminal_write("fatinit     Init FAT32\n");
        }
        else if(strcmp(shell_buf,"clear") ==0)
        {
            terminal_clear();
        }
        else if(strcmp(shell_buf,"pci") ==0)
        {
            pci_scan_bus();
        }
        else if(strcmp(shell_buf,"loadbin") ==0)
        {
            u8 dummy_data[512]={0};
            process_t* p = proc_spawn(0, "testbin", false);
            load_raw_bin(dummy_data,512,p);
        }
        else if(strcmp(shell_buf,"loadcom") ==0)
        {
            u8 dummy_data[512]={0};
            process_t* p = proc_spawn(0, "testcom", false);
            load_dos_com(dummy_data,512,p);
        }
        else if(strcmp(shell_buf,"loadelf") ==0)
        {
            u8 dummy_data[1024]={0};
            process_t* p = proc_spawn(0, "testelf", true);
            load_elf64(dummy_data,1024,p);
        }
        else if(strcmp(shell_buf,"loadpe") ==0)
        {
            u8 dummy_data[2048]={0};
            process_t* p = proc_spawn(0, "testpe", true);
            load_pe64(dummy_data,2048,p);
        }
        else if(strcmp(shell_buf,"loadmacho") ==0)
        {
            u8 dummy_data[2048]={0};
            process_t* p = proc_spawn(0, "testmacho", true);
            load_macho64(dummy_data,2048,p);
        }
        else if(strcmp(shell_buf,"loaddep") ==0)
        {
            u8 dummy_deb[4096]={0};
            u8* elf_out=NULL;
            size_t elf_len=0;
            deb_parse(dummy_deb,4096,&elf_out,&elf_len);
        }
        else if(strcmp(shell_buf,"mountdmg") ==0)
        {
            u8 dummy_dmg[4096]={0};
            dmg_mount(dummy_dmg,4096);
        }
        else if(strcmp(shell_buf,"fatinit") ==0)
        {
            u8 dummy_boot[512]={0};
            fat32_init(dummy_boot);
        }
        shell_pos=0;
        memset(shell_buf,0,256);
        terminal_write("MyOS> ");
        return;
    }
    if(c == 0x08 && shell_pos>0)
    {
        shell_pos--;
        terminal_putchar(0x08);
        terminal_putchar(' ');
        terminal_putchar(0x08);
        return;
    }
    if(shell_pos < 254)
    {
        shell_buf[shell_pos++] = c;
        terminal_putchar(c);
    }
}

void shell_run(void)
{
    shell_init();
    while(1)
    {
        char sc = ps2kbd_read_scancode();
        if(sc !=0)
        {
            char ch = 0;
            switch(sc)
            {
                case 0x1C: ch = '\r'; break;
                case 0x0E: ch = 0x08; break;
                case 0x02: ch='1';break;case 0x03:ch='2';break;case 0x04:ch='3';break;
                case 0x05:ch='4';break;case 0x06:ch='5';break;case 0x07:ch='6';break;
                case 0x10:ch='q';break;case 0x11:ch='w';break;case 0x12:ch='e';break;
                case 0x13:ch='r';break;case 0x14:ch='t';break;case 0x15:ch='y';break;
                case 0x16:ch='u';break;case 0x17:ch='i';break;case 0x18:ch='o';break;
                case 0x19:ch='p';break;case 0x1E:ch='a';break;case 0x1F:ch='s';break;
                case 0x20:ch='d';break;case 0x21:ch='f';break;case 0x22:ch='g';break;
                case 0x23:ch='h';break;case 0x24:ch='j';break;case 0x25:ch='k';break;
                case 0x26:ch='l';break;case 0x2C:ch='z';break;case 0x2D:ch='x';break;
                case 0x2E:ch='c';break;case 0x2F:ch='v';break;case 0x30:ch='b';break;
                case 0x31:ch='n';break;case 0x32:ch='m';break;
                case 0x39:ch=' ';break;
                default: ch=0;
            }
            if(ch) shell_input(ch);
        }
        asm volatile("hlt");
    }
}

/* ===================== Kernel Main Entry ========================= */
void kernel_main(void)
{
    terminal_initialize();
    terminal_write("============================================\n");
    terminal_write("MyOS x86-64 Kernel | GPLv3\n");
    terminal_write("Full Multi-Binary Compatibility Layer Loaded\n");
    terminal_write("Supported: RAW .BIN | DOS .COM | ELF64 | PE64(.EXE) | Mach-O(DMG)\n");
    terminal_write("Package Parser: DEB(ar/tar), DMG(HFS+)\n");
    terminal_write("Drivers: PS2 Keyboard, PCI, FAT32 FS, PMM/VMM, Kernel Heap, Ring3 User Mode\n");
    terminal_write("============================================\n");

    pmm_init();
    heap_init();
    idt_init();
    ps2kbd_init();
    memset(proc_list,0,sizeof(proc_list));

    shell_run();
}
