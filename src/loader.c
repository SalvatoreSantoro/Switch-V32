#include "loader.h"
#include "cpu.h"
#include "defs.h"
#include "macros.h"
#include "memory.h"
#include "threads_mgr.h"
#include <assert.h>
#include <elf.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#pragma GCC diagnostic error "-Wconversion"

#define VALIDATE(cond)                                                                                                 \
    do {                                                                                                               \
        if (!(cond))                                                                                                   \
            SV32_CRASH("ELF HEADER INVALID\n");                                                                        \
    } while (0)

extern Threads_Mgr threads_mgr;

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

static void ld_elf_validate(Elf_File *elf) {
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

static Elf32_Shdr *ld_elf_getsect(Elf_File *elf, const char *sect_name) {
    Elf32_Shdr *sect;
    for (int i = 0; i < elf->header->e_shnum; i++) {
        sect = &elf->sections[i];
        // the section name is offset inside the string of names
        if (strcmp(sect_name, elf->sections_names + sect->sh_name) == 0)
            return sect;
    }
    return NULL;
}

static Elf32_Sym *ld_elf_getsym(Elf_File *elf, const char *sym_name) {
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

static void ld_elf_section_names(Elf_File *elf) {
    Elf32_Shdr shstrtab;
    uint32_t idx = elf->header->e_shstrndx;

    if (idx >= SHN_LORESERVE)
        idx = elf->sections[0].sh_link;

    shstrtab = elf->sections[idx];

    elf->sections_names = (const char *) (elf->data + shstrtab.sh_offset);
}

static void ld_elf_symbols(Elf_File *elf) {
    Elf32_Shdr *sect;

    // load symbols table
    sect = ld_elf_getsect(elf, ".symtab");
    if (sect == NULL)
        SV32_CRASH("CAN'T FIND .symtab\n");

    elf->symb_size = sect->sh_size / sect->sh_entsize;
    elf->symbols = (Elf32_Sym *) (elf->data + sect->sh_offset);

    // load symbols names
    sect = ld_elf_getsect(elf, ".strtab");
    if (sect == NULL)
        SV32_CRASH("CAN'T FIND .strtab\n");
    elf->symbols_names = (const char *) (elf->data + sect->sh_offset);
}

/* static void _ld_find_section */

static void ld_elf_seg(Elf_File *elf) {
    uint32_t fsz;
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
        LOG_LOAD();

        mem_wb_ptr_s(addr, (elf->data + foff), memsz);
        // for bss section
        // memory is currently static so should already be to 0, but just in case...
        if (fsz < memsz) {
            mem_wb_s((addr + fsz), 0, (memsz - fsz));
        }
    }
}

static uint8_t *ld_read_and_map_file(int *fd, struct stat *fst) {
    uint8_t *data;

    if ((*fd = open(ctx.elf_name, O_RDONLY)) == -1)
        SV32_CRASH(strerror(errno));

    if (fstat(*fd, fst) == -1)
        SV32_CRASH(strerror(errno));

    if (fst->st_size <= 0)
        SV32_CRASH("File has a non positive length");

    if ((data = mmap(NULL, (size_t) fst->st_size, PROT_READ, MAP_PRIVATE, *fd, 0)) == NULL)
        SV32_CRASH(strerror(errno));

    return data;
}

static void load_unmap_and_close_file(uint8_t *data, const int *fd, const struct stat *fst) {
    if (fst->st_size <= 0)
        SV32_CRASH("File has a non positive length");

    munmap(data, (size_t) fst->st_size);
    close(*fd);
}

void ld_bin(VCore *core) {
    int fd;
    struct stat fst;
    uint8_t *data;

    data = ld_read_and_map_file(&fd, &fst);

    // load binary
    mem_wb_ptr_s(0, data, (size_t) fst.st_size);

    core->pc = 0;

    load_unmap_and_close_file(data, &fd, &fst);
}

void ld_elf(VCore *core) {
    Elf32_Sym *sym;
    Elf_File elf;
    int fd;
    struct stat fst;

    elf.data = ld_read_and_map_file(&fd, &fst);

    elf.header = (Elf32_Ehdr *) elf.data;

    // crash if the format is incorrect
    ld_elf_validate(&elf);

    elf.programs = (Elf32_Phdr *) (elf.data + elf.header->e_phoff);
    elf.sections = (Elf32_Shdr *) (elf.data + elf.header->e_shoff);
    elf.prog_size = elf.header->e_phentsize * elf.header->e_phnum;
    elf.sect_size = elf.header->e_shentsize * elf.header->e_shnum;

    // init section names string
    ld_elf_section_names(&elf);

    // init symbols string
    ld_elf_symbols(&elf);

    // load segments
    ld_elf_seg(&elf);

    // load entry point + STACK_BASE
	core->pc = elf.header->e_entry;
	core->regs[SP] = STACK_BASE;

    // load GP
    sym = ld_elf_getsym(&elf, "__global_pointer$");

    if (sym == NULL)
        fprintf(stderr, "[WARNING] CAN'T FIND SYMBOL __global_pointer$, relying on program init routine to set GP\n");
    else
        core->regs[GP] = sym->st_value;

    // load brk
    sym = ld_elf_getsym(&elf, "__BSS_END__");
    if (sym == NULL) {
        fprintf(stderr, "[WARNING] CAN'T FIND SYMBOL __BSS_END__, automatically computing brk value\n");
        Elf32_Phdr last_segment = elf.programs[elf.prog_size - 1];
        core->elf_brk = last_segment.p_vaddr + last_segment.p_memsz;
    } else
        core->elf_brk = sym->st_value;

    // load errno
    // seems broken...
    sym = ld_elf_getsym(&elf, "errno");
    if (sym == NULL)
        fprintf(stderr, "[WARNING] CAN'T FIND ERRNO\n");
    else
        core->elf_errno = sym->st_value;

    load_unmap_and_close_file(elf.data, &fd, &fst);
    return;
}
