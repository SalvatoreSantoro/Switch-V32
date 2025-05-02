#include "system.h"
#include "cpu.h"
#include "macros.h"
#include "memory.h"
#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

// System calls needed from Newlib

#define CLOSE         57
#define LSEEK         62
#define READ          63
#define WRITE         64
#define FSTAT         80
#define EXIT          93
#define GETTIMEOFDAY  169
#define BRK           214
#define CLOCK_GETTIME 403
#define OPEN          1024

// supposing 1024 maximum file descriptors
// 0-511 for this program 512-1023 for the elf

#define ELF_FDS_BASELINE 512

#define LOADER_CRASH(str)                                                                                              \
    do {                                                                                                               \
        fprintf(stderr, "The ELF loader crashed: %s\n", str);                                                          \
        exit(EXIT_FAILURE);                                                                                            \
    } while (0)

#define VALIDATE(cond)                                                                                                 \
    do {                                                                                                               \
        if (!(cond))                                                                                                   \
            LOADER_CRASH("ELF HEADER INVALID\n");                                                                      \
    } while (0)

typedef struct {
    size_t prog_size;
    size_t sect_size;
    size_t symb_size;
    uint8_t *data;
    Elf32_Phdr *programs;
    Elf32_Shdr *sections;
    Elf32_Ehdr *header;
    Elf32_Sym *symbols;
    const char *symbols_names;
    const char *sections_names;
} Elf_File;

// need to "translate" x86-64 struct stat to rv32
// referring to Newlib implementation of "sys/stat.h"
struct EMU_timespec {
    int64_t tv_sec;  /* seconds */
    int32_t tv_nsec; /* and nanoseconds */
};

struct EMU_stat {
    int16_t st_dev;
    uint16_t st_ino;
    uint32_t st_mode;
    uint16_t st_nlink;
    uint16_t st_uid;
    uint16_t st_gid;
    int16_t st_rdev;
    int32_t st_size;
    struct EMU_timespec st_atim;
    struct EMU_timespec st_mtim;
    struct EMU_timespec st_ctim;
    int32_t st_blksize;
    int32_t st_blocks;
    int32_t st_spare4[2];
};

static void _emu_copy_timespec(struct EMU_timespec *ets, struct timespec *ts) {
    ets->tv_sec = ts->tv_sec;
    ets->tv_nsec = ts->tv_nsec;
}

static void _emu_copy_stat(struct EMU_stat *es, struct stat *fs) {
    es->st_dev = fs->st_dev;
    es->st_ino = fs->st_ino;
    es->st_mode = fs->st_mode;
    es->st_nlink = fs->st_nlink;
    es->st_uid = fs->st_uid;
    es->st_gid = fs->st_gid;
    es->st_rdev = fs->st_rdev;
    es->st_size = fs->st_size;
    _emu_copy_timespec(&es->st_atim, &fs->st_atim);
    _emu_copy_timespec(&es->st_mtim, &fs->st_mtim);
    _emu_copy_timespec(&es->st_ctim, &fs->st_ctim);
    es->st_blksize = fs->st_blksize;
    es->st_blocks = fs->st_blocks;
}

static void _ld_elf_validate(Elf_File *elf) {
    VALIDATE(elf->header->e_ident[EI_MAG0] == ELFMAG0);
    VALIDATE(elf->header->e_ident[EI_MAG1] == ELFMAG1);
    VALIDATE(elf->header->e_ident[EI_MAG2] == ELFMAG2);
    VALIDATE(elf->header->e_ident[EI_MAG3] == ELFMAG3);
    VALIDATE(elf->header->e_ident[EI_CLASS] == ELFCLASS32);
    VALIDATE(elf->header->e_ident[EI_CLASS] == ELFDATA2LSB);
    VALIDATE(elf->header->e_ident[EI_VERSION] == EV_CURRENT);
    VALIDATE(elf->header->e_ident[EI_OSABI] == ELFOSABI_SYSV);
    VALIDATE(elf->header->e_ident[EI_ABIVERSION] == 0);
    VALIDATE(elf->header->e_type == ET_EXEC);
    VALIDATE(elf->header->e_machine == EM_RISCV);
    VALIDATE(elf->header->e_entry != 0);
    VALIDATE(elf->header->e_phoff != 0);
    VALIDATE(elf->header->e_shoff != 0);
    VALIDATE(elf->header->e_shstrndx != SHN_UNDEF);
}

static Elf32_Shdr *_ld_elf_getsect(Elf_File *elf, const char *sect_name) {
    Elf32_Shdr *sect;
    for (int i = 0; i < elf->header->e_shnum; i++) {
        sect = &elf->sections[i];
        // the section name is offset inside the string of names
        if (strcmp(sect_name, elf->sections_names + sect->sh_name) == 0)
            return sect;
    }
    return NULL;
}

static Elf32_Sym *_ld_elf_getsym(Elf_File *elf, const char *sym_name) {
    Elf32_Sym *sym;
    for (size_t i = 0; i < elf->symb_size; i++) {
        sym = &elf->symbols[i];
        if (strcmp(sym_name, elf->symbols_names + sym->st_name) == 0)
            return sym;
    }
    return NULL;
}

// Citing elf(5) man page
// If the index of section name string table section is larger
// than or equal to SHN_LORESERVE (0xff00), this member holds
// SHN_XINDEX (0xffff) and the real index of the section name
// string table section is held in the sh_link member of the
// initial entry in section header table.  Otherwise, the
// sh_link member of the initial entry in section header table
// contains the value zero.

static void _ld_elf_section_names(Elf_File *elf) {
    Elf32_Shdr shstrtab;
    uint32_t idx = elf->header->e_shstrndx;

    if (idx >= SHN_LORESERVE)
        idx = elf->sections[0].sh_link;

    shstrtab = elf->sections[idx];

    elf->sections_names = (const char *) (elf->data + shstrtab.sh_offset);
}

static void _ld_elf_symbols(Elf_File *elf) {
    Elf32_Shdr *sect;

    // load symbols table
    sect = _ld_elf_getsect(elf, ".symtab");
    if (sect == NULL)
        LOADER_CRASH("CAN'T FIND .symtab\n");

    elf->symb_size = sect->sh_size / sect->sh_entsize;
    elf->symbols = (Elf32_Sym *) (elf->data + sect->sh_offset);

    // load symbols names
    sect = _ld_elf_getsect(elf, ".strtab");
    if (sect == NULL)
        LOADER_CRASH("CAN'T FIND .strtab\n");
    elf->symbols_names = (const char *) (elf->data + sect->sh_offset);
}

/* static void _ld_find_section */

static void _ld_elf_seg(Elf_File *elf) {
    size_t fsz;
    uint32_t foff;
    uint32_t memsz;
    uint32_t addr;

    for (int i = 0; i < elf->header->e_phnum; i++) {
        if (elf->programs[i].p_type != PT_LOAD)
            continue;

        fsz = elf->programs[i].p_filesz;
        memsz = elf->programs[i].p_memsz;
        foff = elf->programs[i].p_offset;
        addr = elf->programs[i].p_vaddr;

        mem_wb_ptr_s(addr, (elf->data + foff), memsz);
        // for bss section
        // memory is currently static so should already be to 0, but just in case...
        if (fsz < memsz) {
            mem_wb_s((addr + fsz), 0, (memsz - fsz));
        }

        LOG_LOAD();
    }
}

void ld_elf(const char *file_name, VCore *core) {
    Elf32_Sym *sym;
    Elf_File *elf = NULL;
    int fd;
    struct stat fst;

    if ((elf = calloc(1, sizeof(Elf_File))) == NULL)
        LOADER_CRASH(strerror(errno));

    // load the file
    if ((fd = open(file_name, O_RDONLY)) == -1)
        LOADER_CRASH(strerror(errno));

    if (fstat(fd, &fst) == -1)
        LOADER_CRASH(strerror(errno));

    if ((elf->data = mmap(NULL, fst.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == NULL)
        LOADER_CRASH(strerror(errno));

    elf->header = (Elf32_Ehdr *) elf->data;

    // crash if the format is incorrect
    _ld_elf_validate(elf);

    elf->programs = (Elf32_Phdr *) (elf->data + elf->header->e_phoff);
    elf->sections = (Elf32_Shdr *) (elf->data + elf->header->e_shoff);
    elf->prog_size = elf->header->e_phentsize * elf->header->e_phnum;
    elf->sect_size = elf->header->e_shentsize * elf->header->e_shnum;

    // init section names string
    _ld_elf_section_names(elf);

    // init symbols string
    _ld_elf_symbols(elf);

    // load segments
    _ld_elf_seg(elf);

    // load entry point
    core->pc = elf->header->e_entry;

    // load stack base
    core->regs[SP] = STACK_BASE;

    // load GP
    sym = _ld_elf_getsym(elf, "__global_pointer$");

    if (sym == NULL)
        fprintf(stderr, "[WARNING] CAN'T FIND SYMBOL __global_pointer$, relying on program init routine to set GP\n");
    else
        core->regs[GP] = sym->st_value;

    // load brk
    sym = _ld_elf_getsym(elf, "__BSS_END__");
    if (sym == NULL) {
        fprintf(stderr, "[WARNING] CAN'T FIND SYMBOL __BSS_END__, automatically computing brk value\n");
        Elf32_Phdr last_segment = elf->programs[elf->prog_size - 1];
        core->elf_brk = last_segment.p_vaddr + last_segment.p_memsz;
    } else
        core->elf_brk = sym->st_value;

    // load errno
    sym = _ld_elf_getsym(elf, "errno");
    if (sym == NULL)
        fprintf(stderr, "[WARNING] CAN'T FIND ERRNO\n");
    else
        core->elf_errno = sym->st_value;

    munmap(elf->data, fst.st_size);
    close(fd);
    free(elf);
    return;
}

void ld_elf_std(const char *stdin_name, const char *stdout_name, const char *stderr_name) {
    int ret;
    int fd;

    if (stdin_name == NULL)
        ret = dup2(STDIN_FILENO, ELF_FDS_BASELINE);
    else {
        fd = open(stdin_name, O_RDONLY);
        ret = dup2(fd, ELF_FDS_BASELINE);
        close(fd);
    }
    if (ret == -1)
        LOADER_CRASH("CAN'T SET STDIN");

    if (stdout_name == NULL)
        dup2(STDOUT_FILENO, ELF_FDS_BASELINE + 1);
    else {
        fd = open(stdout_name, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        ret = dup2(fd, ELF_FDS_BASELINE + 1);
        close(fd);
    }
    if (ret == -1)
        LOADER_CRASH("CAN'T SET STDOUT");

    if (stderr_name == NULL)
        dup2(STDERR_FILENO, ELF_FDS_BASELINE + 2);
    else {
        fd = open(stderr_name, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        ret = dup2(fd, ELF_FDS_BASELINE + 2);
        close(fd);
    }
    if (ret == -1)
        LOADER_CRASH("CAN'T SET STDERR");
}

void ld_elf_args(int elf_argc, ...) {
    const char *str;
    size_t str_size = 0;
    size_t str_size_inc = 0;
    va_list ap;

    // the layout is |STACK_BASE|ARGC|ARGV[0]|ARGV[1]|...|ARGV[ARGC-1]|NULL|STR0|STR1|...|STR_ARGC-1|
    mem_ww(STACK_BASE, elf_argc);

    uint32_t arg_str_start = STACK_BASE + 4 + ((elf_argc + 1) * 4);
    uint32_t arg_ptr_start = STACK_BASE + 4;

    va_start(ap, elf_argc);
    for (int i = 0; i < elf_argc; i++) {
        str = va_arg(ap, const char *);
        str_size = strlen(str) + 1; // keep in mind the '\0'
        mem_wb_ptr_s(arg_str_start + str_size_inc, str, str_size);
        mem_ww(arg_ptr_start + (i * 4), arg_str_start + str_size_inc);
        str_size_inc += str_size;
    }

    mem_ww(arg_ptr_start + (elf_argc * 4), 0);
    va_end(ap);
}

void system_call(VCore *core) {
    int fd;
    struct stat fs;
    struct timespec ts;
    uintptr_t tmp_addr;
    uintptr_t tmp_addr2;
    switch (core->regs[A7]) {
    case CLOSE:
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("CLOSE", fd);
        core->regs[A0] = close(fd);
        break;
    case LSEEK:
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("LSEEK", fd);
        core->regs[A0] = lseek(fd, (off_t) core->regs[A1], (int) core->regs[A2]);
        break;
    case READ:
        tmp_addr = core->regs[A1];
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("READ", fd);
        core->regs[A0] = read(fd, (void *) (__vmem.m + tmp_addr), core->regs[A2]);
        break;
    case WRITE:
        tmp_addr = core->regs[A1];
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("WRITE", fd);
        core->regs[A0] = write(fd, (void *) (__vmem.m + tmp_addr), core->regs[A2]);
        break;
    case FSTAT:
        fd = ELF_FDS_BASELINE + core->regs[A0];
        LOG_DE("FSTAT", fd);
        tmp_addr = core->regs[A1];
        core->regs[A0] = fstat(fd, &fs);
        _emu_copy_stat((struct EMU_stat *) (__vmem.m + tmp_addr), &fs);
        break;
    case GETTIMEOFDAY:
        LOG_DE("GETTIMEOFDAY UNIMPLEMENTED", 0);
        break;
        tmp_addr = core->regs[A0];
        tmp_addr2 = core->regs[A1];
        core->regs[A0] = gettimeofday((struct timeval *) (__vmem.m + tmp_addr), (void *) tmp_addr2);
        break;
    case EXIT:
        LOG_DE("EXIT", core->regs[A0]);
        tmp_addr = (uintptr_t) (__vmem.m + core->elf_errno);
        if (tmp_addr != 0)
            fprintf(stderr, "Last application ERRNO: %d %s\n", *((int *) tmp_addr), strerror(*(int *) tmp_addr));
        exit(core->regs[A0]);
        break;
    case CLOCK_GETTIME:
        tmp_addr = core->regs[A1];
        core->regs[A0] = clock_gettime(core->regs[A0], &ts);
        _emu_copy_timespec((struct EMU_timespec *) (__vmem.m + tmp_addr), &ts);
        LOG_DE("CLOCK_GETTIME", core->regs[A0]);
        break;
    case OPEN:
        tmp_addr = core->regs[A0];
        LOG_STR("OPEN", (const char *) (__vmem.m + tmp_addr));
        fd = open((const char *) (__vmem.m + tmp_addr), core->regs[A1]);
        core->regs[A0] = fd;
        if (fd >= ELF_FDS_BASELINE) {
            fprintf(stderr, "THE ELF FILE OPENED TOO MANY FD\n");
            exit(EXIT_FAILURE);
        }
        dup2(fd, ELF_FDS_BASELINE + fd);
        close(fd);
        break;
    case BRK:
        LOG_EX("BRK", core->regs[A0]);
        if (core->regs[A0] > 0)
            core->elf_brk = core->regs[A0];
        core->regs[A0] = core->elf_brk;
        break;
    default:
        fprintf(stderr, "UNSUPPORTED SYSCALL %d at %x\n", core->regs[A7], core->pc);
        // core->regs[A0] = 0;
        // exit(EXIT_FAILURE);
    }
}
